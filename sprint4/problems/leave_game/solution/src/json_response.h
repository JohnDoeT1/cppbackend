#pragma once

#include <boost/json.hpp>
#include <string>

#include "extra_data.h"
#include "model.h"
#include "retired_players_repository.h"

namespace json_response {

namespace json = boost::json;

// ---------- Карта ----------

inline json::value RoadToJson(const model::Road& road) {
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

inline json::value BuildingToJson(const model::Building& building) {
    const auto& bounds = building.GetBounds();
    json::object obj;
    obj["x"] = bounds.position.x;
    obj["y"] = bounds.position.y;
    obj["w"] = bounds.size.width;
    obj["h"] = bounds.size.height;
    return obj;
}

inline json::value OfficeToJson(const model::Office& office) {
    json::object obj;
    obj["id"] = *office.GetId();
    obj["x"] = office.GetPosition().x;
    obj["y"] = office.GetPosition().y;
    obj["offsetX"] = office.GetOffset().dx;
    obj["offsetY"] = office.GetOffset().dy;
    return obj;
}

inline json::value MapToJson(const model::Map& map,
                             const extra_data::LootTypes& loot_types,
                             bool full) {
    json::object obj;
    obj["id"] = *map.GetId();
    obj["name"] = map.GetName();

    if (!full) {
        return obj;
    }

    json::array roads;
    for (const auto& road : map.GetRoads()) {
        roads.push_back(RoadToJson(road));
    }
    obj["roads"] = std::move(roads);

    json::array buildings;
    for (const auto& building : map.GetBuildings()) {
        buildings.push_back(BuildingToJson(building));
    }
    obj["buildings"] = std::move(buildings);

    json::array offices;
    for (const auto& office : map.GetOffices()) {
        offices.push_back(OfficeToJson(office));
    }
    obj["offices"] = std::move(offices);

    if (const auto* loot_types_value = loot_types.FindForMap(*map.GetId())) {
        obj["lootTypes"] = *loot_types_value;
    }
    return obj;
}

// ---------- Состояние игры ----------

inline json::value PlayerToJson(const model::Player& player) {
    const auto& dog = player.GetDog();
    const auto& pos = dog->GetPosition();
    const auto& speed = dog->GetSpeed();

    json::object player_state;

    json::array pos_arr;
    pos_arr.push_back(pos.x);
    pos_arr.push_back(pos.y);
    player_state["pos"] = std::move(pos_arr);

    json::array speed_arr;
    speed_arr.push_back(speed.x);
    speed_arr.push_back(speed.y);
    player_state["speed"] = std::move(speed_arr);

    std::string direction_str;
    switch (dog->GetDirection()) {
        case model::Direction::NORTH:
            direction_str = "U";
            break;
        case model::Direction::SOUTH:
            direction_str = "D";
            break;
        case model::Direction::WEST:
            direction_str = "L";
            break;
        case model::Direction::EAST:
            direction_str = "R";
            break;
    }
    player_state["dir"] = direction_str;

    json::array bag_arr;
    for (const auto& item : player.GetBag()) {
        json::object bag_item;
        bag_item["id"] = *item.id;
        bag_item["type"] = item.type;
        bag_arr.push_back(std::move(bag_item));
    }
    player_state["bag"] = std::move(bag_arr);

    player_state["score"] = player.GetScore();
    return player_state;
}

inline json::value LostObjectToJson(const model::LostObject& lost_object) {
    json::object obj;
    obj["type"] = lost_object.type;
    json::array pos_arr;
    pos_arr.push_back(lost_object.pos.x);
    pos_arr.push_back(lost_object.pos.y);
    obj["pos"] = std::move(pos_arr);
    return obj;
}

// ---------- Таблица рекордов ----------

inline json::value RetiredPlayerToJson(
    const infrastructure::RetiredPlayerRecord& record) {
    json::object obj;
    obj["name"] = record.name;
    obj["score"] = record.score;
    obj["playTime"] = record.play_time;
    return obj;
}

}  // namespace json_response
