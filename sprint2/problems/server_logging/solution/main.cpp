#include "logging.h"
#include "logging_request_handler.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <thread>
#include <exception>
#include <memory>
#include <sstream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// Функция для создания JSON строки
std::string MakeJson(const std::string& key, const std::string& value) {
    return "\"" + key + "\":\"" + value + "\"";
}

// Ваш оригинальный RequestHandler
class RequestHandler {
public:
    http::response<http::string_body> operator()(http::request<http::string_body>&& req) {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "text/html");
        res.body() = "<html><body>Hello World!</body></html>";
        res.prepare_payload();
        return res;
    }
};

// Функция для логирования сетевых ошибок
void LogNetworkError(const beast::error_code& ec, const std::string& where) {
    if (ec == net::error::eof || ec == beast::error::timeout) {
        return; // Не логируем нормальное завершение соединения
    }
    
    std::ostringstream data;
    data << "{" << MakeJson("code", std::to_string(ec.value())) << ","
         << MakeJson("text", ec.message()) << ","
         << MakeJson("where", where) << "}";
    
    BOOST_LOG_TRIVIAL(error) 
        << logging::add_value(additional_data, data.str())
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
        std::string ip = socket_.remote_endpoint().address().to_string();
        LogRequest(ip, request_);
        
        auto start = std::chrono::steady_clock::now();
        auto response = handler_(std::move(request_));
        auto end = std::chrono::steady_clock::now();
        auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        LogResponse(ip, response, response_time);
        WriteResponse(response);
    }

    void LogRequest(const std::string& ip, const http::request<http::string_body>& req) {
        std::ostringstream data;
        data << "{" << MakeJson("ip", ip) << ","
             << MakeJson("URI", std::string(req.target())) << ","
             << MakeJson("method", std::string(req.method_string())) << "}";
        
        BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(additional_data, data.str())
            << "request received";
    }

    void LogResponse(const std::string& ip, const http::response<http::string_body>& response, int64_t response_time) {
        std::ostringstream data;
        data << "{" << MakeJson("ip", ip) << ","
             << "\"response_time\":" << response_time << ","
             << "\"code\":" << response.result_int() << ",";
        
        auto content_type = response.find(http::field::content_type);
        if (content_type != response.end()) {
            data << MakeJson("content_type", std::string(content_type->value()));
        } else {
            data << "\"content_type\":null";
        }
        data << "}";
        
        BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(additional_data, data.str())
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
                beast::error_code shutdown_ec;
                self->socket_.shutdown(tcp::socket::shutdown_send, shutdown_ec);
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
        
        std::ostringstream start_data;
        start_data << "{\"port\":" << port << ",\"address\":\"" << argv[1] << "\"}";
        
        BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(additional_data, start_data.str())
            << "server started";
        
        net::io_context ioc;
        tcp::acceptor acceptor{ioc, {address, port}};
        
        RequestHandler handler;
        LoggingRequestHandler<RequestHandler> logging_handler(handler);
        
        // Основной цикл обработки соединений
        while (true) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            
            auto session = std::make_shared<Session>(std::move(socket), logging_handler);
            session->Run();
            
            // Запускаем обработку асинхронных операций
            ioc.run();
            ioc.restart();
        }
        
    } catch (const std::exception& e) {
        std::ostringstream error_data;
        error_data << "{\"code\":" << EXIT_FAILURE << ",\"exception\":\"" << e.what() << "\"}";
        
        BOOST_LOG_TRIVIAL(error) 
            << logging::add_value(additional_data, error_data.str())
            << "server exited";
        
        return EXIT_FAILURE;
    }
    
    return 0;
}
