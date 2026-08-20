#pragma once
#include "http_server.h"
#include "model.h"
#include "static_content.h"

#include <string>
#include <sstream>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, const std::string& static_path)
        : game_{game}, static_handler_(static_path) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto target = std::string(req.target());
        
        // Обработка статических файлов
        if (!target.starts_with("/api/")) {
            if (req.method() == http::verb::get || req.method() == http::verb::head) {
                auto static_response = static_handler_.handle_request(target);
                
                auto res = http::response<http::string_body>(
                    static_cast<http::status>(static_response.status_code), 11);
                res.set(http::field::content_type, static_response.content_type);
                res.set(http::field::content_length, std::to_string(static_response.content_length));
                
                if (req.method() == http::verb::get) {
                    res.body() = static_response.body;
                }
                
                send(std::move(res));
                return;
            } else {
                auto res = http::response<http::string_body>(http::status::method_not_allowed, 11);
                res.set(http::field::content_type, "text/plain");
                res.body() = "Method Not Allowed";
                res.content_length(res.body().size());
                send(std::move(res));
                return;
            }
        }
        
        // Обработка API запросов
        if (req.method() != http::verb::get) {
            auto res = http::response<http::string_body>(http::status::bad_request, 11);
            res.set(http::field::content_type, "application/json");
            res.body() = "{\"code\":\"badRequest\",\"message\":\"Bad request\"}";
            res.content_length(res.body().size());
            send(std::move(res));
            return;
        }
        
        if (target == "/api/v1/maps") {
            std::string json = "[";
            bool first = true;
            for (const auto& map : game_.GetMaps()) {
                if (!first) json += ",";
                json += "{\"id\":\"" + *map.GetId() + "\",\"name\":\"" + map.GetName() + "\"}";
                first = false;
            }
            json += "]";
            
            auto res = http::response<http::string_body>(http::status::ok, 11);
            res.set(http::field::content_type, "application/json");
            res.body() = json;
            res.content_length(res.body().size());
            send(std::move(res));
            return;
        }
        
        if (target.starts_with("/api/v1/maps/")) {
            auto id = target.substr(13);
            auto* map = game_.FindMap(model::Map::Id(id));
            if (map) {
                std::string json = "{\"id\":\"" + *map->GetId() + "\",\"name\":\"" + map->GetName() + "\",\"roads\":[";
                bool first = true;
                for (const auto& r : map->GetRoads()) {
                    if (!first) json += ",";
                    json += "{\"x0\":" + std::to_string(r.GetStart().x) + 
                            ",\"y0\":" + std::to_string(r.GetStart().y);
                    if (r.IsHorizontal()) {
                        json += ",\"x1\":" + std::to_string(r.GetEnd().x);
                    } else {
                        json += ",\"y1\":" + std::to_string(r.GetEnd().y);
                    }
                    json += "}";
                    first = false;
                }
                json += "],\"buildings\":[";
                first = true;
                for (const auto& b : map->GetBuildings()) {
                    if (!first) json += ",";
                    json += "{\"x\":" + std::to_string(b.GetBounds().position.x) +
                            ",\"y\":" + std::to_string(b.GetBounds().position.y) +
                            ",\"w\":" + std::to_string(b.GetBounds().size.width) +
                            ",\"h\":" + std::to_string(b.GetBounds().size.height) + "}";
                    first = false;
                }
                json += "],\"offices\":[";
                first = true;
                for (const auto& o : map->GetOffices()) {
                    if (!first) json += ",";
                    json += "{\"id\":\"" + *o.GetId() + "\"" +
                            ",\"x\":" + std::to_string(o.GetPosition().x) +
                            ",\"y\":" + std::to_string(o.GetPosition().y) +
                            ",\"offsetX\":" + std::to_string(o.GetOffset().dx) +
                            ",\"offsetY\":" + std::to_string(o.GetOffset().dy) + "}";
                    first = false;
                }
                json += "]}";
                
                auto res = http::response<http::string_body>(http::status::ok, 11);
                res.set(http::field::content_type, "application/json");
                res.body() = json;
                res.content_length(res.body().size());
                send(std::move(res));
            } else {
                auto res = http::response<http::string_body>(http::status::not_found, 11);
                res.set(http::field::content_type, "application/json");
                res.body() = "{\"code\":\"mapNotFound\",\"message\":\"Map not found\"}";
                res.content_length(res.body().size());
                send(std::move(res));
            }
            return;
        }
        
        auto res = http::response<http::string_body>(http::status::bad_request, 11);
        res.set(http::field::content_type, "application/json");
        res.body() = "{\"code\":\"badRequest\",\"message\":\"Bad request\"}";
        res.content_length(res.body().size());
        send(std::move(res));
    }

private:
    model::Game& game_;
    StaticContentHandler static_handler_;
};

}  // namespace http_handler
