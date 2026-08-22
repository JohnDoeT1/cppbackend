#pragma once
#include "http_server.h"
#include "model.h"
#include "static_content.h"

#include <boost/json.hpp>
#include <sstream>
#include <iomanip>
#include <random>
#include <cctype>

namespace json = boost::json;

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
        
        // Обработка API запросов - важно проверять точные пути первыми
        if (target == "/api/v1/game/join") {
            HandleJoinGame(std::move(req), std::forward<Send>(send));
            return;
        }
        
        if (target == "/api/v1/game/players") {
            HandleGetPlayers(std::move(req), std::forward<Send>(send));
            return;
        }
        
        if (target == "/api/v1/game/state") {
            HandleGetState(std::move(req), std::forward<Send>(send));
            return;
        }
        
        if (target == "/api/v1/game/player/action") {
            HandlePlayerAction(std::move(req), std::forward<Send>(send));
            return;
        }
        
        if (target == "/api/v1/maps") {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                SendMethodNotAllowed(http::verb::get, std::forward<Send>(send));
                return;
            }
            json::array arr;
            for (const auto& map : game_.GetMaps()) {
                arr.push_back(MapToJson(map, false));
            }
            auto res = http::response<http::string_body>(http::status::ok, 11);
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(arr);
            res.content_length(res.body().size());
            send(std::move(res));
            return;
        }
        
        // Проверяем /api/v1/maps/ только если это действительно maps
        if (target.starts_with("/api/v1/maps/")) {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                SendMethodNotAllowed(http::verb::get, std::forward<Send>(send));
                return;
            }
            auto id = target.substr(13);  // длина "/api/v1/maps/" = 13
            auto* map = game_.FindMap(model::Map::Id(id));
            if (map) {
                auto res = http::response<http::string_body>(http::status::ok, 11);
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MapToJson(*map, true));
                res.content_length(res.body().size());
                send(std::move(res));
            } else {
                SendError(http::status::not_found, "mapNotFound", "Map not found", std::forward<Send>(send));
            }
            return;
        }
        
        auto res = http::response<http::string_body>(http::status::bad_request, 11);
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = MakeError("badRequest", "Bad request");
        res.content_length(res.body().size());
        send(std::move(res));
    }

private:
    // Извлечение и валидация токена
    std::optional<std::string> ExtractToken(const http::request<http::string_body>& req) {
        auto auth_header = req.find(http::field::authorization);
        if (auth_header == req.end()) {
            return std::nullopt;
        }
        
        std::string auth_value = std::string(auth_header->value());
        const std::string bearer_prefix = "Bearer ";
        if (auth_value.size() < bearer_prefix.size() || 
            auth_value.substr(0, bearer_prefix.size()) != bearer_prefix) {
            return std::nullopt;
        }
        
        std::string token = auth_value.substr(bearer_prefix.size());
        
        // Проверяем длину токена - должна быть ровно 32 символа
        if (token.size() != 32) {
            return std::nullopt;
        }
        
        // Проверяем, что токен содержит только hex-символы
        for (char c : token) {
            if (!std::isxdigit(static_cast<unsigned char>(c))) {
                return std::nullopt;
            }
        }
        
        return token;
    }
    
    template <typename Send>
    void HandleJoinGame(http::request<http::string_body>&& req, Send&& send) {
        if (req.method() != http::verb::post) {
            SendMethodNotAllowed(http::verb::post, std::forward<Send>(send));
            return;
        }
        
        try {
            json::value parsed = json::parse(req.body());
            if (!parsed.is_object()) {
                SendError(http::status::bad_request, "invalidArgument", "Join game request parse error", std::forward<Send>(send));
                return;
            }
            
            json::object obj = parsed.as_object();
            
            if (!obj.contains("userName") || !obj.contains("mapId")) {
                SendError(http::status::bad_request, "invalidArgument", "Join game request parse error", std::forward<Send>(send));
                return;
            }
            
            std::string user_name = json::value_to<std::string>(obj["userName"]);
            std::string map_id_str = json::value_to<std::string>(obj["mapId"]);
            
            if (user_name.empty()) {
                SendError(http::status::bad_request, "invalidArgument", "Invalid name", std::forward<Send>(send));
                return;
            }
            
            model::Map::Id map_id(map_id_str);
            if (!game_.FindMap(map_id)) {
                SendError(http::status::not_found, "mapNotFound", "Map not found", std::forward<Send>(send));
                return;
            }
            
            uint32_t player_id = game_.AddPlayer(user_name, map_id);
            std::string auth_token = GenerateToken();
            game_.SetPlayerToken(auth_token, player_id);
            
            json::object response_obj;
            response_obj["authToken"] = auth_token;
            response_obj["playerId"] = player_id;
            
            auto res = http::response<http::string_body>(http::status::ok, 11);
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(response_obj);
            res.content_length(res.body().size());
            send(std::move(res));
            
        } catch (const std::exception&) {
            SendError(http::status::bad_request, "invalidArgument", "Join game request parse error", std::forward<Send>(send));
        }
    }
    
    template <typename Send>
    void HandleGetPlayers(http::request<http::string_body>&& req, Send&& send) {
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            SendMethodNotAllowed(http::verb::get, std::forward<Send>(send));
            return;
        }
        
        auto token_opt = ExtractToken(req);
        if (!token_opt.has_value()) {
            SendError(http::status::unauthorized, "invalidToken", "Authorization header is missing", std::forward<Send>(send));
            return;
        }
        
        auto player_id = game_.GetPlayerIdByToken(*token_opt);
        if (!player_id.has_value()) {
            SendError(http::status::unauthorized, "unknownToken", "Player token has not been found", std::forward<Send>(send));
            return;
        }
        
        const auto& players = game_.GetPlayers();
        auto player_it = players.find(*player_id);
        if (player_it == players.end()) {
            SendError(http::status::unauthorized, "unknownToken", "Player token has not been found", std::forward<Send>(send));
            return;
        }
        
        model::Map::Id map_id = game_.GetPlayerMap(*player_id);
        
        json::object players_json;
        const auto& map_players = game_.GetPlayersOnMap(map_id);
        for (auto pid : map_players) {
            auto pit = players.find(pid);
            if (pit != players.end()) {
                json::object player_obj;
                player_obj["name"] = pit->second.GetName();
                players_json[std::to_string(pid)] = player_obj;
            }
        }
        
        auto res = http::response<http::string_body>(http::status::ok, 11);
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        if (req.method() == http::verb::get) {
            res.body() = json::serialize(players_json);
        }
        res.content_length(res.body().size());
        send(std::move(res));
    }
    
    template <typename Send>
    void HandleGetState(http::request<http::string_body>&& req, Send&& send) {
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            SendMethodNotAllowed(http::verb::get, std::forward<Send>(send));
            return;
        }
        
        auto token_opt = ExtractToken(req);
        if (!token_opt.has_value()) {
            SendError(http::status::unauthorized, "invalidToken", "Authorization header is required", std::forward<Send>(send));
            return;
        }
        
        auto player_id = game_.GetPlayerIdByToken(*token_opt);
        if (!player_id.has_value()) {
            SendError(http::status::unauthorized, "unknownToken", "Player token has not been found", std::forward<Send>(send));
            return;
        }
        
        const auto& players = game_.GetPlayers();
        auto player_it = players.find(*player_id);
        if (player_it == players.end()) {
            SendError(http::status::unauthorized, "unknownToken", "Player token has not been found", std::forward<Send>(send));
            return;
        }
        
        model::Map::Id map_id = game_.GetPlayerMap(*player_id);
        
        json::object players_json;
        const auto& map_players = game_.GetPlayersOnMap(map_id);
        for (auto pid : map_players) {
            auto pit = players.find(pid);
            if (pit != players.end()) {
                const auto& dog = pit->second.GetDog();
                const auto& pos = dog->GetPosition();
                const auto& speed = dog->GetSpeed();
                
                json::object player_state;
                
                json::array pos_arr;
                pos_arr.push_back(pos.x);
                pos_arr.push_back(pos.y);
                player_state["pos"] = pos_arr;
                
                json::array speed_arr;
                speed_arr.push_back(speed.vx);
                speed_arr.push_back(speed.vy);
                player_state["speed"] = speed_arr;
                
                std::string dir_str;
                switch (dog->GetDirection()) {
                    case model::Direction::NORTH:
                        dir_str = "U";
                        break;
                    case model::Direction::SOUTH:
                        dir_str = "D";
                        break;
                    case model::Direction::WEST:
                        dir_str = "L";
                        break;
                    case model::Direction::EAST:
                        dir_str = "R";
                        break;
                }
                player_state["dir"] = dir_str;
                
                players_json[std::to_string(pid)] = player_state;
            }
        }
        
        json::object response_obj;
        response_obj["players"] = players_json;
        
        auto res = http::response<http::string_body>(http::status::ok, 11);
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        if (req.method() == http::verb::get) {
            res.body() = json::serialize(response_obj);
        }
        res.content_length(res.body().size());
        send(std::move(res));
    }
    
    template <typename Send>
    void HandlePlayerAction(http::request<http::string_body>&& req, Send&& send) {
        if (req.method() != http::verb::post) {
            auto res = http::response<http::string_body>(http::status::method_not_allowed, 11);
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.set(http::field::allow, "POST");
            res.body() = MakeError("invalidMethod", "Invalid method");
            res.content_length(res.body().size());
            send(std::move(res));
            return;
        }
        
        // Проверяем Content-Type
        auto content_type = req.find(http::field::content_type);
        if (content_type == req.end() || 
            std::string(content_type->value()) != "application/json") {
            SendError(http::status::bad_request, "invalidArgument", "Invalid content type", std::forward<Send>(send));
            return;
        }
        
        auto token_opt = ExtractToken(req);
        if (!token_opt.has_value()) {
            SendError(http::status::unauthorized, "invalidToken", "Authorization header is required", std::forward<Send>(send));
            return;
        }
        
        auto player_id = game_.GetPlayerIdByToken(*token_opt);
        if (!player_id.has_value()) {
            SendError(http::status::unauthorized, "unknownToken", "Player token has not been found", std::forward<Send>(send));
            return;
        }
        
        try {
            json::value parsed = json::parse(req.body());
            if (!parsed.is_object()) {
                SendError(http::status::bad_request, "invalidArgument", "Failed to parse action", std::forward<Send>(send));
                return;
            }
            
            json::object obj = parsed.as_object();
            if (!obj.contains("move")) {
                SendError(http::status::bad_request, "invalidArgument", "Failed to parse action", std::forward<Send>(send));
                return;
            }
            
            std::string move = json::value_to<std::string>(obj["move"]);
            
            // Проверяем валидность значения move
            if (move != "L" && move != "R" && move != "U" && move != "D" && move != "") {
                SendError(http::status::bad_request, "invalidArgument", "Failed to parse action", std::forward<Send>(send));
                return;
            }
            
            // Устанавливаем направление движения
            game_.SetPlayerDirection(*player_id, move);
            
            // Формируем ответ
            json::object response_obj;
            
            auto res = http::response<http::string_body>(http::status::ok, 11);
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(response_obj);
            res.content_length(res.body().size());
            send(std::move(res));
            
        } catch (const std::exception&) {
            SendError(http::status::bad_request, "invalidArgument", "Failed to parse action", std::forward<Send>(send));
        }
    }
    
    template <typename Send>
    void SendError(http::status status, const std::string& code, const std::string& message, Send&& send) {
        auto res = http::response<http::string_body>(status, 11);
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = MakeError(code, message);
        res.content_length(res.body().size());
        send(std::move(res));
    }
    
    template <typename Send>
    void SendMethodNotAllowed(http::verb method, Send&& send) {
        auto res = http::response<http::string_body>(http::status::method_not_allowed, 11);
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        
        if (method == http::verb::post) {
            res.set(http::field::allow, "POST");
            res.body() = MakeError("invalidMethod", "Only POST method is expected");
        } else {
            res.set(http::field::allow, "GET, HEAD");
            res.body() = MakeError("invalidMethod", "Invalid method");
        }
        
        res.content_length(res.body().size());
        send(std::move(res));
    }
    
    std::string GenerateToken() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        
        std::stringstream ss;
        ss << std::hex;
        for (int i = 0; i < 32; ++i) {
            ss << dis(gen);
        }
        return ss.str();
    }
    
    std::string MakeError(const std::string& code, const std::string& message) {
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
    StaticContentHandler static_handler_;
};

}  // namespace http_handler
