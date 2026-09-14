#include "json_loader.h"

#include <boost/json.hpp>
#include <chrono>
#include <fstream>
#include <memory>
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

// НОВОЕ: добавлен параметр loot_types_count
model::Map ParseMap(const json::object& obj, double default_dog_speed,
                    std::uint32_t loot_types_count) {
    double dog_speed = default_dog_speed;
    if (obj.contains("dogSpeed")) {
        dog_speed = obj.at("dogSpeed").as_double();
    }

    model::Map map(
        model::Map::Id(json::value_to<std::string>(obj.at("id"))),
        json::value_to<std::string>(obj.at("name")),
        dog_speed,
        loot_types_count  // <-- 4-й аргумент
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

LoadedGame LoadGame(const std::filesystem::path& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file");
    }

    std::stringstream ss;
    ss << file.rdbuf();

    json::value value;
    try {
        value = json::parse(ss.str());
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse config file: " + std::string(e.what()));
    }

    const auto& root = value.as_object();

    double default_dog_speed = 1.0;
    if (root.contains("defaultDogSpeed")) {
        default_dog_speed = root.at("defaultDogSpeed").as_double();
    }

    // --- НОВОЕ: создаём генератор трофеев из lootGeneratorConfig ---
    auto loot_gen = std::make_shared<loot_gen::LootGenerator>(
        std::chrono::milliseconds{
            static_cast<int>(root.at("lootGeneratorConfig").at("period").as_double() * 1000)},
        root.at("lootGeneratorConfig").at("probability").as_double());

    // --- НОВОЕ: game теперь принимает генератор ---
    model::Game game(loot_gen, default_dog_speed);

    // --- НОВОЕ: параллельно заполняем extra_data::LootTypes ---
    extra_data::LootTypes loot_types;

    for (const auto& map_json : root.at("maps").as_array()) {
        const auto& map_obj = map_json.as_object();
        const auto map_id_str = json::value_to<std::string>(map_obj.at("id"));

        const auto loot_arr = map_obj.at("lootTypes").as_array();
        const auto loot_types_count = static_cast<std::uint32_t>(loot_arr.size());

        game.AddMap(ParseMap(map_obj, default_dog_speed, loot_types_count));

        // Сохраняем JSON-массив как есть — он нужен только фронтенду.
        loot_types.SetForMap(map_id_str, loot_arr);
    }

    return LoadedGame{std::move(game), std::move(loot_gen), std::move(loot_types)};
}

}  // namespace json_loader
