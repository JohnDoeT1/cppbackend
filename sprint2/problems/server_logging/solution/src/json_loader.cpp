#include "json_loader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace json_loader {

// Простой JSON парсер для конфигурации
namespace {

std::string GetStringValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return "";
    
    pos = json.find(":", pos + search_key.length());
    if (pos == std::string::npos) return "";
    
    pos = json.find("\"", pos + 1);
    if (pos == std::string::npos) return "";
    
    size_t end = json.find("\"", pos + 1);
    if (end == std::string::npos) return "";
    
    return json.substr(pos + 1, end - pos - 1);
}

int GetIntValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return 0;
    
    pos = json.find(":", pos + search_key.length());
    if (pos == std::string::npos) return 0;
    
    // Пропускаем пробелы
    pos++;
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    
    // Читаем число
    std::string number;
    while (pos < json.length() && (isdigit(json[pos]) || json[pos] == '-')) {
        number += json[pos];
        pos++;
    }
    
    return number.empty() ? 0 : std::stoi(number);
}

std::vector<std::string> GetArrayItems(const std::string& json, const std::string& key) {
    std::vector<std::string> items;
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return items;
    
    pos = json.find("[", pos + search_key.length());
    if (pos == std::string::npos) return items;
    
    // Ищем все объекты в массиве
    size_t depth = 0;
    size_t start = std::string::npos;
    
    for (size_t i = pos; i < json.length(); i++) {
        if (json[i] == '{') {
            if (depth == 0) {
                start = i;
            }
            depth++;
        } else if (json[i] == '}') {
            depth--;
            if (depth == 0 && start != std::string::npos) {
                items.push_back(json.substr(start, i - start + 1));
                start = std::string::npos;
            }
        }
    }
    
    return items;
}

} // namespace

model::Road ParseRoad(const std::string& obj) {
    model::Point start{
        GetIntValue(obj, "x0"),
        GetIntValue(obj, "y0")
    };
    if (obj.find("\"x1\"") != std::string::npos) {
        return model::Road(model::Road::HORIZONTAL, start, 
                          GetIntValue(obj, "x1"));
    }
    return model::Road(model::Road::VERTICAL, start,
                      GetIntValue(obj, "y1"));
}

model::Building ParseBuilding(const std::string& obj) {
    return model::Building(model::Rectangle{
        {GetIntValue(obj, "x"),
         GetIntValue(obj, "y")},
        {GetIntValue(obj, "w"),
         GetIntValue(obj, "h")}
    });
}

model::Office ParseOffice(const std::string& obj) {
    return model::Office(
        model::Office::Id(GetStringValue(obj, "id")),
        {GetIntValue(obj, "x"),
         GetIntValue(obj, "y")},
        {GetIntValue(obj, "offsetX"),
         GetIntValue(obj, "offsetY")}
    );
}

model::Map ParseMap(const std::string& obj) {
    model::Map map(
        model::Map::Id(GetStringValue(obj, "id")),
        GetStringValue(obj, "name")
    );
    
    for (const auto& road : GetArrayItems(obj, "roads")) {
        map.AddRoad(ParseRoad(road));
    }
    for (const auto& building : GetArrayItems(obj, "buildings")) {
        map.AddBuilding(ParseBuilding(building));
    }
    for (const auto& office : GetArrayItems(obj, "offices")) {
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
    for (const auto& map : GetArrayItems(content, "maps")) {
        game.AddMap(ParseMap(map));
    }
    
    return game;
}

}  // namespace json_loader
