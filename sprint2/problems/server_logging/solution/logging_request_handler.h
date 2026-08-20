#pragma once
#include "logging.h"
#include <boost/beast/http.hpp>
#include <chrono>
#include <string>
#include <sstream>

namespace http = boost::beast::http;

// Декоратор для логирования запросов
template<class RequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(RequestHandler& handler) 
        : decorated_(handler) {}

    http::response<http::string_body> operator()(http::request<http::string_body>&& req) {
        auto start = std::chrono::steady_clock::now();
        auto response = decorated_(std::move(req));
        auto end = std::chrono::steady_clock::now();
        
        return response;
    }

private:
    RequestHandler& decorated_;
};
