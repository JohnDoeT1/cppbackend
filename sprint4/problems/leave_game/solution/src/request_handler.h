#pragma once
#include "http_server.h"
#include "model.h"
#include "static_content.h"
#include "extra_data.h"
#include "retired_players_repository.h"
#include "json_response.h"

#include <boost/json.hpp>
#include <sstream>
#include <iomanip>
#include <random>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <vector>

namespace json = boost::json;

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

namespace {

// Эндпоинты API
constexpr std::string_view API_JOIN = "/api/v1/game/join";
constexpr std::string_view API_PLAYERS = "/api/v1/game/players";
constexpr std::string_view API_STATE = "/api/v1/game/state";
constexpr std::string_view API_ACTION = "/api/v1/game/player/action";
constexpr std::string_view API_TICK = "/api/v1/game/tick";
constexpr std::string_view API_MAPS = "/api/v1/maps";
constexpr std::string_view API_RECORDS = "/api/v1/game/records";
constexpr std::string_view API_MAPS_PREFIX = "/api/v1/maps/";

// Ограничения
constexpr int kDefaultMaxItems = 100;
constexpr int kMaxItemsLimit = 100;

// Токен
constexpr std::string_view kBearerPrefix = "Bearer ";
constexpr std::size_t kTokenLength = 32;

// Смещение в /api/v1/maps/<id>
constexpr std::size_t kMapsPrefixLength = API_MAPS_PREFIX.size();

}  // namespace

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game,
                            extra_data::LootTypes& loot_types,
                            infrastructure::RetiredPlayersRepository& records_repo,
                            const std::string& static_path,
                            bool allow_tick = true)
        : game_{game}
        , loot_types_{loot_types}
        , records_repo_{records_repo}
        , static_handler_(static_path)
        , allow_tick_(allow_tick)
        , gen_(random_device_()) {
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
            }
            auto res = http::response<http::string_body>(http::status::method_not_allowed, 11);
            res.set(http::field::content_type, "text/plain");
            res.body() = "Method Not Allowed";
            res.content_length(res.body().size());
            send(std::move(res));
            return;
        }

        // Обработка API запросов — важно проверять точные пути первыми
        if (target == API_JOIN) {
            HandleJoinGame(std::move(req), std::forward<Send>(send));
            return;
        }

        if (target == API_PLAYERS) {
            HandleGetPlayers(std::move(req), std::forward<Send>(send));
            return;
        }

        if (target == API_STATE) {
            HandleGetState(std::move(req), std::forward<Send>(send));
            return;
        }

        if (target == API_ACTION) {
            HandlePlayerAction(std::move(req), std::forward<Send>(send));
            return;
        }

        if (target == API_TICK) {
            if (!allow_tick_) {
                SendError(http::status::bad_request, "badRequest", "Invalid endpoint",
                          std::forward<Send>(send));
                return;
            }
            HandleTick(std::move(req), std::forward<Send>(send));
            return;
        }

        // Запросы с query-параметрами начинаются с API_RECORDS
        if (target.starts_with(API_RECORDS)) {
            HandleRecords(std::move(req), std::forward<Send>(send));
            return;
        }

        if (target == API_MAPS) {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                SendMethodNotAllowed(http::verb::get, std::forward<Send>(send));
                return;
            }
            json::array arr;
            for (const auto& map : game_.GetMaps()) {
                arr.push_back(json_response::MapToJson(map, loot_types_, false));
            }
            auto res = http::response<http::string_body>(http::status::ok, 11);
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(arr);
            res.content_length(res.body().size());
            send(std::move(res));
            return;
        }

        if (target.starts_with(API_MAPS_PREFIX)) {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                SendMethodNotAllowed(http::verb::get, std::forward<Send>(send));
                return;
            }
            const auto id = target.substr(kMapsPrefixLength);
            const auto* map = game_.FindMap(model::Map::Id(id));
            if (map) {
                auto res = http::response<http::string_body>(http::status::ok, 11);
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(json_response::MapToJson(*map, loot_types_, true));
                res.content_length(res.body().size());
                send(std::move(res));
            } else {
                SendError(http::status::not_found, "mapNotFound", "Map not found",
                          std::forward<Send>(send));
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
    static std::unordered_map<std::string, std::string>
    ParseQuery(std::string_view target) {
        std::unordered_map<std::string, std::string> params;
        auto pos = target.find('?');
        if (pos == std::string_view::npos) {
            return params;
        }
        auto query = target.substr(pos + 1);
        while (!query.empty()) {
            auto amp = query.find('&');
            auto pair = query.substr(0, amp);
            auto eq = pair.find('=');
            if (eq != std::string_view::npos) {
                params[std::string(pair.substr(0, eq))] =
                    std::string(pair.substr(eq + 1));
            }
            if (amp == std::string_view::npos) {
                break;
            }
            query.remove_prefix(amp + 1);
        }
        return params;
    }

    template <typename Send>
    void HandleRecords(http::request<http::string_body>&& req, Send&& send) {
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            SendMethodNotAllowed(http::verb::get, std::forward<Send>(send));
            return;
        }

        int start = 0;
        int max_items = kDefaultMaxItems;

        auto params = ParseQuery(std::string(req.target()));

        auto parse_int = [](const std::string& text, int& value) -> bool {
            if (text.empty()) {
                return false;
            }
            try {
                std::size_t parsed_chars = 0;
                const int parsed_value = std::stoi(text, &parsed_chars);
                if (parsed_chars != text.size() || parsed_value < 0) {
                    return false;
                }
                value = parsed_value;
                return true;
            } catch (const std::invalid_argument&) {
                // Строка не является числом
                return false;
            } catch (const std::out_of_range&) {
                // Число не помещается в int
                return false;
            }
        };

        if (auto it = params.find("start"); it != params.end()) {
            if (!parse_int(it->second, start)) {
                SendError(http::status::bad_request, "invalidArgument",
                          "Invalid start parameter", std::forward<Send>(send));
                return;
            }
        }
        if (auto it = params.find("maxItems"); it != params.end()) {
            if (!parse_int(it->second, max_items)) {
                SendError(http::status::bad_request, "invalidArgument",
                          "Invalid maxItems parameter", std::forward<Send>(send));
                return;
            }
            if (max_items > kMaxItemsLimit) {
                SendError(http::status::bad_request, "invalidArgument",
                          "maxItems must not exceed " + std::to_string(kMaxItemsLimit),
                          std::forward<Send>(send));
                return;
            }
        }

        std::vector<infrastructure::RetiredPlayerRecord> records;
        try {
            records = records_repo_.GetRecords(start, max_items);
        } catch (const std::exception&) {
            SendError(http::status::internal_server_error, "internalError",
                      "Failed to read records", std::forward<Send>(send));
            return;
        }

        json::array arr;
        for (const auto& record : records) {
            arr.push_back(json_response::RetiredPlayerToJson(record));
        }

        auto res = http::response<http::string_body>(http::status::ok, 11);
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        if (req.method() == http::verb::get) {
            res.body() = json::serialize(arr);
        }
        res.content_length(res.body().size());
        send(std::move(res));
    }

    std::optional<std::string> ExtractToken(const http::request<http::string_body>& req) {
        const auto auth_header = req.find(http::field::authorization);
        if (auth_header == req.end()) {
            return std::nullopt;
        }

        const std::string auth_value{auth_header->value()};
        if (auth_value.size() < kBearerPrefix.size() ||
            auth_value.compare(0, kBearerPrefix.size(), kBearerPrefix) != 0) {
            return std::nullopt;
        }

        std::string token = auth_value.substr(kBearerPrefix.size());
        if (token.size() != kTokenLength) {
            return std::nullopt;
        }
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
                SendError(http::status::bad_request, "invalidArgument",
                          "Join game request parse error", std::forward<Send>(send));
                return;
            }
            json::object obj = parsed.as_object();
            if (!obj.contains("userName") || !obj.contains("mapId")) {
                SendError(http::status::bad_request, "invalidArgument",
                          "Join game request parse error", std::forward<Send>(send));
                return;
            }
            std::string user_name = json::value_to<std::string>(obj["userName"]);
            std::string map_id_str = json::value_to<std::string>(obj["mapId"]);
            if (user_name.empty()) {
                SendError(http::status::bad_request, "invalidArgument",
                          "Invalid name", std::forward<Send>(send));
                return;
            }
            model::Map::Id map_id(map_id_str);
            if (!game_.FindMap(map_id)) {
                SendError(http::status::not_found, "mapNotFound", "Map not found",
                          std::forward<Send>(send));
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
            SendError(http::status::bad_request, "invalidArgument",
                      "Join game request parse error", std::forward<Send>(send));
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
            SendError(http::status::unauthorized, "invalidToken",
                      "Authorization header is missing", std::forward<Send>(send));
            return;
        }
        auto player_id = game_.GetPlayerIdByToken(*token_opt);
        if (!player_id.has_value()) {
            SendError(http::status::unauthorized, "unknownToken",
                      "Player token has not been found", std::forward<Send>(send));
            return;
        }
        const auto& players = game_.GetPlayers();
        auto player_it = players.find(*player_id);
        if (player_it == players.end()) {
            SendError(http::status::unauthorized, "unknownToken",
                      "Player token has not been found", std::forward<Send>(send));
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
            SendError(http::status::unauthorized, "invalidToken",
                      "Authorization header is required", std::forward<Send>(send));
            return;
        }
        auto player_id = game_.GetPlayerIdByToken(*token_opt);
        if (!player_id.has_value()) {
            SendError(http::status::unauthorized, "unknownToken",
                      "Player token has not been found", std::forward<Send>(send));
            return;
        }
        const auto& players = game_.GetPlayers();
        auto player_it = players.find(*player_id);
        if (player_it == players.end()) {
            SendError(http::status::unauthorized, "unknownToken",
                      "Player token has not been found", std::forward<Send>(send));
            return;
        }
        model::Map::Id map_id = game_.GetPlayerMap(*player_id);
        json::object players_json;
        const auto& map_players = game_.GetPlayersOnMap(map_id);
        for (auto pid : map_players) {
            auto pit = players.find(pid);
            if (pit != players.end()) {
                players_json[std::to_string(pid)] =
                    json_response::PlayerToJson(pit->second);
            }
        }
        json::object lost_objects_json;
        for (const auto& [id, obj] : game_.GetLostObjects().Get()) {
            lost_objects_json[std::to_string(id)] =
                json_response::LostObjectToJson(obj);
        }
        json::object response_obj;
        response_obj["players"] = players_json;
        response_obj["lostObjects"] = lost_objects_json;

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
        auto content_type = req.find(http::field::content_type);
        if (content_type == req.end() ||
            std::string(content_type->value()) != "application/json") {
            SendError(http::status::bad_request, "invalidArgument",
                      "Invalid content type", std::forward<Send>(send));
            return;
        }
        auto token_opt = ExtractToken(req);
        if (!token_opt.has_value()) {
            SendError(http::status::unauthorized, "invalidToken",
                      "Authorization header is required", std::forward<Send>(send));
            return;
        }
        auto player_id = game_.GetPlayerIdByToken(*token_opt);
        if (!player_id.has_value()) {
            SendError(http::status::unauthorized, "unknownToken",
                      "Player token has not been found", std::forward<Send>(send));
            return;
        }
        try {
            json::value parsed = json::parse(req.body());
            if (!parsed.is_object()) {
                SendError(http::status::bad_request, "invalidArgument",
                          "Failed to parse action", std::forward<Send>(send));
                return;
            }
            json::object obj = parsed.as_object();
            if (!obj.contains("move")) {
                SendError(http::status::bad_request, "invalidArgument",
                          "Failed to parse action", std::forward<Send>(send));
                return;
            }
            std::string move = json::value_to<std::string>(obj["move"]);
            if (move != "L" && move != "R" && move != "U" && move != "D" && move != "") {
                SendError(http::status::bad_request, "invalidArgument",
                          "Failed to parse action", std::forward<Send>(send));
                return;
            }
            game_.SetPlayerDirection(*player_id, move);
            json::object response_obj;
            auto res = http::response<http::string_body>(http::status::ok, 11);
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(response_obj);
            res.content_length(res.body().size());
            send(std::move(res));
        } catch (const std::exception&) {
            SendError(http::status::bad_request, "invalidArgument",
                      "Failed to parse action", std::forward<Send>(send));
        }
    }

    template <typename Send>
    void HandleTick(http::request<http::string_body>&& req, Send&& send) {
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
        auto content_type = req.find(http::field::content_type);
        if (content_type == req.end() ||
            std::string(content_type->value()) != "application/json") {
            SendError(http::status::bad_request, "invalidArgument",
                      "Invalid content type", std::forward<Send>(send));
            return;
        }
        try {
            json::value parsed = json::parse(req.body());
            if (!parsed.is_object()) {
                SendError(http::status::bad_request, "invalidArgument",
                          "Failed to parse tick request JSON", std::forward<Send>(send));
                return;
            }
            json::object obj = parsed.as_object();
            if (!obj.contains("timeDelta")) {
                SendError(http::status::bad_request, "invalidArgument",
                          "Failed to parse tick request JSON", std::forward<Send>(send));
                return;
            }
            int64_t time_delta = obj["timeDelta"].as_int64();
            if (time_delta < 0) {
                SendError(http::status::bad_request, "invalidArgument",
                          "Failed to parse tick request JSON", std::forward<Send>(send));
                return;
            }
            game_.Tick(static_cast<int>(time_delta));
            json::object response_obj;
            auto res = http::response<http::string_body>(http::status::ok, 11);
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(response_obj);
            res.content_length(res.body().size());
            send(std::move(res));
        } catch (const std::exception&) {
            SendError(http::status::bad_request, "invalidArgument",
                      "Failed to parse tick request JSON", std::forward<Send>(send));
        }
    }

    template <typename Send>
    void SendError(http::status status, const std::string& code,
                   const std::string& message, Send&& send) {
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
        std::uniform_int_distribution<> dis(0, 15);
        std::stringstream ss;
        ss << std::hex;
        for (int i = 0; i < 32; ++i) {
            ss << dis(gen_);
        }
        return ss.str();
    }

    std::string MakeError(const std::string& code, const std::string& message) {
        json::object obj;
        obj["code"] = code;
        obj["message"] = message;
        return json::serialize(obj);
    }

    model::Game& game_;
    extra_data::LootTypes& loot_types_;
    infrastructure::RetiredPlayersRepository& records_repo_;
    StaticContentHandler static_handler_;
    bool allow_tick_;
    std::random_device random_device_;
    std::mt19937 gen_{random_device_()};
};

}  // namespace http_handler
