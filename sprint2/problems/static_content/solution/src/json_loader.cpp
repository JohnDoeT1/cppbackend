#include "json_loader.h"

#include <boost/json.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace json = boost::json;

namespace json_loader {

model::Road ParseRoad(const json::object& obj) {
    model::Point start{
        static_cast<int>(obj.at("x0").as_int64()),
        static_cast<int>(obj.at("y0").as_int64())
    };
    if (obj.contains("x1")) {
        return model::Road(model::Road::HORIZONTAL, start, 
                          static_cast<int>(obj.at("x1").as_int64()));
    }
    return model::Road(model::Road::VERTICAL, start,
                      static_cast<int>(obj.at("y1").as_int64()));
}

model::Building ParseBuilding(const json::object& obj) {
    return model::Building(model::Rectangle{
        {static_cast<int>(obj.at("x").as_int64()),
         static_cast<int>(obj.at("y").as_int64())},
        {static_cast<int>(obj.at("w").as_int64()),
         static_cast<int>(obj.at("h").as_int64())}
    });
}

model::Office ParseOffice(const json::object& obj) {
    return model::Office(
        model::Office::Id(json::value_to<std::string>(obj.at("id"))),
        {static_cast<int>(obj.at("x").as_int64()),
         static_cast<int>(obj.at("y").as_int64())},
        {static_cast<int>(obj.at("offsetX").as_int64()),
         static_cast<int>(obj.at("offsetY").as_int64())}
    );
}

model::Map ParseMap(const json::object& obj) {
    model::Map map(
        model::Map::Id(json::value_to<std::string>(obj.at("id"))),
        json::value_to<std::string>(obj.at("name"))
    );
    
    for (const auto& road : obj.at("roads").as_array()) {
        map.AddRoad(ParseRoad(road.as_object()));
    }
    for (const auto& building : obj.at("buildings").as_array()) {
        map.AddBuilding(ParseBuilding(building.as_object()));
    }
    for (const auto& office : obj.at("offices").as_array()) {
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
    for (const auto& map : value.at("maps").as_array()) {
        game.AddMap(ParseMap(map.as_object()));
    }
    
    return game;
}

}  // namespace json_loader
