#pragma once

#include "http_server.h"
#include <boost/json.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <chrono>
#include <string_view>

namespace http_handler {

namespace json = boost::json;
namespace beast = boost::beast;
namespace http = beast::http;

template<typename RequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(RequestHandler& decorated)
        : decorated_(decorated) {
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto start_time = std::chrono::steady_clock::now();
        
        // Логируем получение запроса
        LogRequest(req);
        
        // Обрабатываем запрос
        decorated_(std::move(req), [this, start_time, send = std::forward<Send>(send)](auto&& response) mutable {
            // Логируем формирование ответа
            LogResponse(response, start_time);
            
            // Отправляем ответ
            send(std::move(response));
        });
    }

private:
    template <typename Body, typename Allocator>
    void LogRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        json::value custom_data{
            {"ip", "127.0.0.1"},
            {"URI", std::string(req.target())},
            {"method", std::string(req.method_string())}
        };
        
        BOOST_LOG_SEV(http_server::lg, boost::log::trivial::info) 
            << boost::log::add_value("AdditionalData", custom_data)
            << "request received";
    }
    
    template <typename Body, typename Fields>
    void LogResponse(const http::response<Body, Fields>& response, 
                     std::chrono::steady_clock::time_point start_time) {
        auto end_time = std::chrono::steady_clock::now();
        auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        json::value custom_data{
            {"ip", "127.0.0.1"},
            {"response_time", static_cast<int64_t>(response_time)},
            {"code", static_cast<int>(response.result_int())}
        };
        
        // Добавляем content_type если он есть
        if (auto it = response.find(http::field::content_type); it != response.end()) {
            custom_data.as_object()["content_type"] = std::string(it->value());
        } else {
            custom_data.as_object()["content_type"] = nullptr;
        }
        
        BOOST_LOG_SEV(http_server::lg, boost::log::trivial::info) 
            << boost::log::add_value("AdditionalData", custom_data)
            << "response sent";
    }

    RequestHandler& decorated_;
};

}  // namespace http_handler
