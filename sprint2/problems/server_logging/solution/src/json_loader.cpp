#include "json_loader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <vector>
#include <regex>

namespace json_loader {

// Простой JSON парсер (только для нашей конфигурации)
class SimpleJson {
public:
    static std::string GetString(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return match[1].str();
        }
        return "";
    }
    
    static int GetInt(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*(\\d+)");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return std::stoi(match[1].str());
        }
        return 0;
    }
    
    static std::vector<std::string> GetArray(const std::string& json, const std::string& key) {
        std::vector<std::string> result;
        std::regex pattern("\"" + key + "\"\\s*:\\s*\\[(.*?)\\]");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            std::string array_content = match[1].str();
            std::regex obj_pattern("\\{[^}]*\\}");
            std::smatch obj_match;
            std::string::const_iterator search_start = array_content.cbegin();
            while (std::regex_search(search_start, array_content.cend(), obj_match, obj_pattern)) {
                result.push_back(obj_match[0].str());
                search_start = obj_match.suffix().first;
            }
        }
        return result;
    }
};

model::Road ParseRoad(const std::string& obj) {
    model::Point start{
        SimpleJson::GetInt(obj, "x0"),
        SimpleJson::GetInt(obj, "y0")
    };
    if (obj.find("\"x1\"") != std::string::npos) {
        return model::Road(model::Road::HORIZONTAL, start, 
                          SimpleJson::GetInt(obj, "x1"));
    }
    return model::Road(model::Road::VERTICAL, start,
                      SimpleJson::GetInt(obj, "y1"));
}

model::Building ParseBuilding(const std::string& obj) {
    return model::Building(model::Rectangle{
        {SimpleJson::GetInt(obj, "x"),
         SimpleJson::GetInt(obj, "y")},
        {SimpleJson::GetInt(obj, "w"),
         SimpleJson::GetInt(obj, "h")}
    });
}

model::Office ParseOffice(const std::string& obj) {
    return model::Office(
        model::Office::Id(SimpleJson::GetString(obj, "id")),
        {SimpleJson::GetInt(obj, "x"),
         SimpleJson::GetInt(obj, "y")},
        {SimpleJson::GetInt(obj, "offsetX"),
         SimpleJson::GetInt(obj, "offsetY")}
    );
}

model::Map ParseMap(const std::string& obj) {
    model::Map map(
        model::Map::Id(SimpleJson::GetString(obj, "id")),
        SimpleJson::GetString(obj, "name")
    );
    
    for (const auto& road : SimpleJson::GetArray(obj, "roads")) {
        map.AddRoad(ParseRoad(road));
    }
    for (const auto& building : SimpleJson::GetArray(obj, "buildings")) {
        map.AddBuilding(ParseBuilding(building));
    }
    for (const auto& office : SimpleJson::GetArray(obj, "offices")) {
        map.AddOffice(ParseOffice(office));
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
    std::string content = ss.str();
    
    model::Game game;
    for (const auto& map : SimpleJson::GetArray(content, "maps")) {
        game.AddMap(ParseMap(map));
    }
    
    return game;
}

}  // namespace json_loader
