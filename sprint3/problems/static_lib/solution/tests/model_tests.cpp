#include <catch2/catch_test_macros.hpp>

#include "../src/loot_generator.h"
#include "../src/model.h"

#include <chrono>
#include <memory>

using namespace std::chrono_literals;

namespace {

model::Map MakeMap(std::uint32_t loot_types_count) {
    model::Map map{model::Map::Id{"m"}, "M", 1.0, loot_types_count};
    // Одна горизонтальная дорога от (0, 10) до (20, 10)
    map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 10}, 20});
    return map;
}

}  // namespace

SCENARIO("Game generates lost objects on tick", "[model][loot]") {
    GIVEN("A game with a generator that always generates 1 item per tick") {
        // probability=1, rng=1 -> генерация срабатывает
        auto gen = std::make_shared<loot_gen::LootGenerator>(
            1s, 1.0, [] { return 1.0; });
        model::Game game{gen, 1.0};

        auto map = MakeMap(3);
        game.AddMap(std::move(map));

        WHEN("Tick is called with a player on the map") {
            // Добавим игрока, чтобы looter_count был > 0
            const auto player_id = game.AddPlayer("dog", model::Map::Id{"m"});
            game.Tick(1000);

            THEN("at least one lost object is created") {
                CHECK(game.GetLostObjects().Size() >= 1);
            }

            THEN("the object's type is in [0, 3)") {
                for (const auto& [_, obj] : game.GetLostObjects().Get()) {
                    CHECK(obj.type < 3);
                }
            }

            THEN("the object is located on the road (y == 10)") {
                for (const auto& [_, obj] : game.GetLostObjects().Get()) {
                    CHECK(obj.pos.y == 10.0);
                    CHECK(obj.pos.x >= 0.0);
                    CHECK(obj.pos.x <= 20.0);
                }
            }
        }

        WHEN("Tick is called without players") {
            game.Tick(1000);

            THEN("no objects are created because there are no looters") {
                CHECK(game.GetLostObjects().Size() == 0);
            }
        }
    }
}

SCENARIO("LootGenerator returns zero when loot >= looters", "[loot_generator]") {
    GIVEN("Generator with probability=1") {
        loot_gen::LootGenerator gen{1s, 1.0, [] { return 1.0; }};

        THEN("Generate returns 0 when loot_count >= looter_count") {
            CHECK(gen.Generate(1s, 5, 5) == 0);
            CHECK(gen.Generate(1s, 10, 3) == 0);
        }

        THEN("Generate returns positive when loot_count < looter_count") {
            const auto n = gen.Generate(1s, 0, 3);
            CHECK(n > 0);
            CHECK(n <= 3);
        }
    }
}
