#include "logging.h"
#include "logging_request_handler.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <thread>
#include <exception>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// Ваш оригинальный RequestHandler
class RequestHandler {
public:
    http::response<http::string_body> operator()(http::request<http::string_body>&& req) {
        // Ваша логика обработки запроса
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "text/html");
        res.body() = "<html><body>Hello World!</body></html>";
        res.prepare_payload();
        return res;
    }
};

// Функция для логирования сетевых ошибок
void LogNetworkError(const beast::error_code& ec, const std::string& where) {
    json::object data;
    data["code"] = ec.value();
    data["text"] = json::string(ec.message());
    data["where"] = json::string(where);
    
    BOOST_LOG_TRIVIAL(error) 
        << logging::add_value(additional_data, data)
        << "error";
}

// Класс сессии
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, LoggingRequestHandler<RequestHandler>& handler)
        : socket_(std::move(socket)), handler_(handler) {}

    void Run() {
        ReadRequest();
    }

private:
    void ReadRequest() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, request_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    LogNetworkError(ec, "read");
                    return;
                }
                self->HandleRequest();
            });
    }

    void HandleRequest() {
        // Получаем IP-адрес клиента
        std::string ip = socket_.remote_endpoint().address().to_string();
        
        // Логируем запрос с IP
        LogRequest(ip, request_);
        
        // Обрабатываем запрос
        auto start = std::chrono::steady_clock::now();
        auto response = handler_(std::move(request_));
        auto end = std::chrono::steady_clock::now();
        auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        // Логируем ответ с IP
        LogResponse(ip, response, response_time);
        
        WriteResponse(response);
    }

    void LogRequest(const std::string& ip, const http::request<http::string_body>& req) {
        json::object data;
        data["ip"] = json::string(ip);
        data["URI"] = json::string(std::string(req.target()));
        data["method"] = json::string(std::string(req.method_string()));
        
        BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(additional_data, data)
            << "request received";
    }

    void LogResponse(const std::string& ip, const http::response<http::string_body>& response, int64_t response_time) {
        json::object data;
        data["ip"] = json::string(ip);
        data["response_time"] = response_time;
        data["code"] = response.result_int();
        
        auto content_type = response.find(http::field::content_type);
        if (content_type != response.end()) {
            data["content_type"] = json::string(std::string(content_type->value()));
        } else {
            data["content_type"] = json::value(nullptr);
        }
        
        BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(additional_data, data)
            << "response sent";
    }

    void WriteResponse(http::response<http::string_body>& response) {
        auto self = shared_from_this();
        http::async_write(socket_, response,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    LogNetworkError(ec, "write");
                    return;
                }
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
    }

    tcp::socket socket_;
    LoggingRequestHandler<RequestHandler>& handler_;
    http::request<http::string_body> request_;
    beast::flat_buffer buffer_;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <address> <port>\n";
        return EXIT_FAILURE;
    }
    
    try {
        InitLogging();
        
        const auto address = net::ip::make_address(argv[1]);
        const auto port = static_cast<unsigned short>(std::atoi(argv[2]));
        
        // Логируем запуск сервера
        json::object start_data;
        start_data["port"] = port;
        start_data["address"] = json::string(argv[1]);
        
        BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(additional_data, start_data)
            << "server started";
        
        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {address, port}};
        
        RequestHandler handler;
        LoggingRequestHandler<RequestHandler> logging_handler(handler);
        
        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::make_shared<Session>(std::move(socket), logging_handler)->Run();
        }
        
    } catch (const std::exception& e) {
        // Логируем ошибку
        json::object error_data;
        error_data["code"] = EXIT_FAILURE;
        error_data["exception"] = json::string(e.what());
        
        BOOST_LOG_TRIVIAL(error) 
            << logging::add_value(additional_data, error_data)
            << "server exited";
        
        return EXIT_FAILURE;
    }
    
    return 0;
}
