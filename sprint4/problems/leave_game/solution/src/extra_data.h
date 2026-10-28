#pragma once

#include <boost/json.hpp>
#include <string>
#include <unordered_map>

namespace extra_data {

// Хранит "сырой" JSON-массив lootTypes для каждой карты.
// Модель об этом ничего не знает — эти данные нужны только REST API.
class LootTypes {
public:
    void SetForMap(const std::string& map_id, boost::json::array loot_types) {
        by_map_[map_id] = std::move(loot_types);
    }

    const boost::json::array* FindForMap(const std::string& map_id) const noexcept {
        const auto it = by_map_.find(map_id);
        return it == by_map_.end() ? nullptr : &it->second;
    }

private:
    std::unordered_map<std::string, boost::json::array> by_map_;
};

}  // namespace extra_data
