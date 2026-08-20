#pragma once
#include "logging.h"
#include <boost/beast/http.hpp>
#include <chrono>
#include <string>

namespace http = boost::beast::http;

// Декоратор для логирования запросов
template<class RequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(RequestHandler& handler) 
        : decorated_(handler) {}

    http::response<http::string_body> operator()(http::request<http::string_body>&& req) {
        // Логируем запрос (без IP, так как мы не знаем его здесь)
        LogRequest(req);
        
        auto start = std::chrono::steady_clock::now();
        auto response = decorated_(std::move(req));
        auto end = std::chrono::steady_clock::now();
        
        auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        LogResponse(response, response_time);
        
        return response;
    }

private:
    void LogRequest(const http::request<http::string_body>& req) {
        json::object data;
        data["URI"] = json::string(std::string(req.target()));
        data["method"] = json::string(std::string(req.method_string()));
        
        BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(additional_data, data)
            << "request received";
    }
    
    void LogResponse(const http::response<http::string_body>& response, int64_t response_time) {
        json::object data;
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
    
    RequestHandler& decorated_;
};
