#ifdef WIN32
#include <sdkddkver.h>
#endif

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <iostream>
#include <thread>
#include <optional>
#include <string>


namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;

namespace beast = boost::beast;
namespace http = beast::http;


using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;


// Чтение HTTP-запроса
std::optional<StringRequest> ReadRequest(
    tcp::socket& socket,
    beast::flat_buffer& buffer
) {
    beast::error_code ec;

    StringRequest request;

    http::read(socket, buffer, request, ec);


    if (ec == http::error::end_of_stream) {
        return std::nullopt;
    }


    if (ec) {
        throw std::runtime_error(
            "Failed to read request: "s + ec.message()
        );
    }


    return request;
}


// Создание HTTP-ответа
StringResponse MakeStringResponse(
    http::status status,
    std::string body,
    unsigned version,
    bool keep_alive
) {

    StringResponse response(status, version);


    response.set(
        http::field::content_type,
        "text/html"sv
    );


    response.body() = std::move(body);


    response.content_length(
        response.body().size()
    );


    response.keep_alive(keep_alive);


    return response;
}


// Обработка HTTP-запросов
StringResponse HandleRequest(StringRequest&& request) {


    auto text_response =
        [&request](http::status status, std::string body) {

            return MakeStringResponse(
                status,
                std::move(body),
                request.version(),
                request.keep_alive()
            );
        };



    if (request.method() == http::verb::get ||
        request.method() == http::verb::head) {


        std::string target =
            std::string(request.target());


        // Удаляем первый символ '/'
        if (!target.empty() && target.front() == '/') {
            target.erase(0, 1);
        }


        std::string body =
            "Hello, " + target;


        auto response =
            text_response(
                http::status::ok,
                body
            );


        // HEAD: тело отсутствует,
        // но Content-Length остаётся
        if (request.method() == http::verb::head) {

            response.body().clear();

        }


        return response;
    }



    auto response =
        text_response(
            http::status::method_not_allowed,
            "Invalid method"
        );


    response.set(
        http::field::allow,
        "GET, HEAD"sv
    );


    return response;
}



// Обработка соединения
template <typename RequestHandler>
void HandleConnection(
    tcp::socket& socket,
    RequestHandler&& request_handler
) {

    try {

        beast::flat_buffer buffer;


        while (auto request = ReadRequest(socket, buffer)) {


            StringResponse response =
                request_handler(
                    std::move(*request)
                );


            http::write(
                socket,
                response
            );


            if (response.need_eof()) {
                break;
            }
        }

    }
    catch (const std::exception& e) {

        std::cerr
            << e.what()
            << std::endl;
    }


    beast::error_code ec;


    socket.shutdown(
        tcp::socket::shutdown_send,
        ec
    );
}



int main() {

    net::io_context ioc;


    auto address =
        net::ip::make_address("0.0.0.0");


    constexpr unsigned short port = 8080;


    tcp::acceptor acceptor(
        ioc,
        {address, port}
    );


    std::cout
        << "Server has started..."
        << std::endl;



    while (true) {


        tcp::socket socket(ioc);


        acceptor.accept(socket);



        std::thread thread(
            [](tcp::socket socket) {

                HandleConnection(
                    socket,
                    HandleRequest
                );

            },
            std::move(socket)
        );


        thread.detach();
    }
}
