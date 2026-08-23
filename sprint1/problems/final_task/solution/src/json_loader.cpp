#include "json_loader.h"

#include <boost/json.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace json = boost::json;

namespace json_loader {

namespace {
// Ключи для доступа к полям JSON
constexpr std::string_view KEY_X = "x";
constexpr std::string_view KEY_Y = "y";
constexpr std::string_view KEY_W = "w";
constexpr std::string_view KEY_H = "h";
constexpr std::string_view KEY_X0 = "x0";
constexpr std::string_view KEY_Y0 = "y0";
constexpr std::string_view KEY_X1 = "x1";
constexpr std::string_view KEY_Y1 = "y1";
constexpr std::string_view KEY_ID = "id";
constexpr std::string_view KEY_NAME = "name";
constexpr std::string_view KEY_OFFSET_X = "offsetX";
constexpr std::string_view KEY_OFFSET_Y = "offsetY";
constexpr std::string_view KEY_ROADS = "roads";
constexpr std::string_view KEY_BUILDINGS = "buildings";
constexpr std::string_view KEY_OFFICES = "offices";
constexpr std::string_view KEY_MAPS = "maps";
} // namespace

model::Road ParseRoad(const json::object& obj) {
    model::Point start{
        static_cast<int>(obj.at(KEY_X0).as_int64()),
        static_cast<int>(obj.at(KEY_Y0).as_int64())
    };
    if (obj.contains(KEY_X1)) {
        return model::Road(model::Road::HORIZONTAL, start, 
                          static_cast<int>(obj.at(KEY_X1).as_int64()));
    }
    return model::Road(model::Road::VERTICAL, start,
                      static_cast<int>(obj.at(KEY_Y1).as_int64()));
}

model::Building ParseBuilding(const json::object& obj) {
    return model::Building(model::Rectangle{
        {static_cast<int>(obj.at(KEY_X).as_int64()),
         static_cast<int>(obj.at(KEY_Y).as_int64())},
        {static_cast<int>(obj.at(KEY_W).as_int64()),
         static_cast<int>(obj.at(KEY_H).as_int64())}
    });
}

model::Office ParseOffice(const json::object& obj) {
    return model::Office(
        model::Office::Id(json::value_to<std::string>(obj.at(KEY_ID))),
        {static_cast<int>(obj.at(KEY_X).as_int64()),
         static_cast<int>(obj.at(KEY_Y).as_int64())},
        {static_cast<int>(obj.at(KEY_OFFSET_X).as_int64()),
         static_cast<int>(obj.at(KEY_OFFSET_Y).as_int64())}
    );
}

model::Map ParseMap(const json::object& obj) {
    model::Map map(
        model::Map::Id(json::value_to<std::string>(obj.at(KEY_ID))),
        json::value_to<std::string>(obj.at(KEY_NAME))
    );
    
    for (const auto& road : obj.at(KEY_ROADS).as_array()) {
        map.AddRoad(ParseRoad(road.as_object()));
    }
    for (const auto& building : obj.at(KEY_BUILDINGS).as_array()) {
        map.AddBuilding(ParseBuilding(building.as_object()));
    }
    for (const auto& office : obj.at(KEY_OFFICES).as_array()) {
        map.AddOffice(ParseOffice(office.as_object()));
    }
    
    return map;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file");
    }
    
    std::stringstream ss;
    ss << file.rdbuf();
    auto value = json::parse(ss.str());
    
    model::Game game;
    for (const auto& map : value.at(KEY_MAPS).as_array()) {
        game.AddMap(ParseMap(map.as_object()));
    }
    
    return game;
}

}  // namespace json_loader
