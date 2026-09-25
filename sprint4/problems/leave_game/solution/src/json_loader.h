#pragma once

#include <chrono>
#include <filesystem>
#include <memory>

#include "extra_data.h"
#include "model.h"

namespace json_loader {

struct LootGeneratorConfig {
    std::chrono::milliseconds period;
    double probability;
};

struct LoadedGame {
    model::Game game;
    std::shared_ptr<loot_gen::LootGenerator> loot_generator;
    extra_data::LootTypes loot_types;
    std::chrono::milliseconds dog_retirement_time{60'000};   // <-- ДОБАВЛЕНО
};

LoadedGame LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
