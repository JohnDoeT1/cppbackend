#pragma once
#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>

namespace json = boost::json;

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto target = std::string(req.target());
        
        if (req.method() != http::verb::get || !target.starts_with("/api/")) {
            auto res = http::response<http::string_body>(http::status::bad_request, 11);
            res.set(http::field::content_type, "application/json");
            res.body() = MakeError("badRequest", "Bad request");
            res.content_length(res.body().size());
            send(std::move(res));
            return;
        }
        
        if (target == "/api/v1/maps") {
            json::array arr;
            for (const auto& map : game_.GetMaps()) {
                arr.push_back(MapToJson(map, false));
            }
            auto res = http::response<http::string_body>(http::status::ok, 11);
            res.set(http::field::content_type, "application/json");
            res.body() = json::serialize(arr);
            res.content_length(res.body().size());
            send(std::move(res));
            return;
        }
        
        if (target.starts_with("/api/v1/maps/")) {
            auto id = target.substr(13);
            auto* map = game_.FindMap(model::Map::Id(id));
            if (map) {
                auto res = http::response<http::string_body>(http::status::ok, 11);
                res.set(http::field::content_type, "application/json");
                res.body() = json::serialize(MapToJson(*map, true));
                res.content_length(res.body().size());
                send(std::move(res));
            } else {
                auto res = http::response<http::string_body>(http::status::not_found, 11);
                res.set(http::field::content_type, "application/json");
                res.body() = MakeError("mapNotFound", "Map not found");
                res.content_length(res.body().size());
                send(std::move(res));
            }
            return;
        }
        
        auto res = http::response<http::string_body>(http::status::bad_request, 11);
        res.set(http::field::content_type, "application/json");
        res.body() = MakeError("badRequest", "Bad request");
        res.content_length(res.body().size());
        send(std::move(res));
    }

private:
    std::string MakeError(std::string_view code, std::string_view message) {
        json::object obj;
        obj["code"] = code;
        obj["message"] = message;
        return json::serialize(obj);
    }

    json::value RoadToJson(const model::Road& road) {
        json::object obj;
        obj["x0"] = road.GetStart().x;
        obj["y0"] = road.GetStart().y;
        if (road.IsHorizontal()) {
            obj["x1"] = road.GetEnd().x;
        } else {
            obj["y1"] = road.GetEnd().y;
        }
        return obj;
    }

    json::value MapToJson(const model::Map& map, bool full) {
        json::object obj;
        obj["id"] = *map.GetId();
        obj["name"] = map.GetName();
        
        if (full) {
            json::array roads;
            for (const auto& r : map.GetRoads()) {
                roads.push_back(RoadToJson(r));
            }
            obj["roads"] = roads;
            
            json::array buildings;
            for (const auto& b : map.GetBuildings()) {
                json::object bo;
                bo["x"] = b.GetBounds().position.x;
                bo["y"] = b.GetBounds().position.y;
                bo["w"] = b.GetBounds().size.width;
                bo["h"] = b.GetBounds().size.height;
                buildings.push_back(bo);
            }
            obj["buildings"] = buildings;
            
            json::array offices;
            for (const auto& o : map.GetOffices()) {
                json::object oo;
                oo["id"] = *o.GetId();
                oo["x"] = o.GetPosition().x;
                oo["y"] = o.GetPosition().y;
                oo["offsetX"] = o.GetOffset().dx;
                oo["offsetY"] = o.GetOffset().dy;
                offices.push_back(oo);
            }
            obj["offices"] = offices;
        }
        
        return obj;
    }

    model::Game& game_;
};

}  // namespace http_handler
