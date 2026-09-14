#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "../src/model.h"

#include <chrono>
#include <memory>

using namespace std::chrono_literals;
using Catch::Matchers::WithinAbs;

namespace {

// Карта с одной горизонтальной дорогой от (0, 10) до (20, 10).
// bag_capacity — вместимость рюкзака для игроков на этой карте.
model::Map MakeMap(std::uint32_t loot_types_count, std::size_t bag_capacity) {
    model::Map map{model::Map::Id{"m"}, "M", 1.0, loot_types_count, bag_capacity};
    map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 10}, 20});
    return map;
}

// Карта с базой в точке base_pos.
model::Map MakeMapWithBase(std::uint32_t loot_types_count,
                           std::size_t bag_capacity,
                           model::Point base_pos) {
    model::Map map{model::Map::Id{"m"}, "M", 1.0, loot_types_count, bag_capacity};
    map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 10}, 20});
    map.AddOffice(model::Office{
        model::Office::Id{"base"},
        base_pos,
        model::Offset{0, 0}
    });
    return map;
}

}  // namespace

// =========================================================================
// 1. Игрок подбирает предмет, мимо которого проходит
// =========================================================================
SCENARIO("Player picks up item on the way", "[find_return][collect]") {
    GIVEN("A game with one player and one item on the path") {
        model::Game game{1.0};
        game.AddMap(MakeMap(2, 3));

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(1, {10.0, 10.0});  // предмет по центру дороги

        REQUIRE(game.GetLostObjects().Size() == 1);

        WHEN("Player moves right until reaching the item") {
            game.SetPlayerDirection(pid, "R");
            for (int i = 0; i < 20; ++i) {
                game.Tick(1000);  // 1 сек, скорость 1.0 -> +1.0 по X
            }

            THEN("The item is in the bag") {
                const auto& bag = game.GetPlayers().at(pid).GetBag();
                REQUIRE(bag.size() == 1);
                CHECK(bag[0].type == 1);
                CHECK(bag[0].id == 0);
            }

            THEN("The item is removed from lostObjects") {
                CHECK(game.GetLostObjects().Size() == 0);
            }
        }
    }
}

// =========================================================================
// 2. Полный рюкзак — предмет пропускается
// =========================================================================
SCENARIO("Player skips item when bag is full", "[find_return][bag_full]") {
    GIVEN("A game with bag capacity 1 and two items on the path") {
        model::Game game{1.0};
        game.AddMap(MakeMap(2, 1));  // вместимость 1

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(0, {5.0, 10.0});
        game.GetLostObjects().Add(0, {10.0, 10.0});

        WHEN("Player moves right past both items") {
            game.SetPlayerDirection(pid, "R");
            for (int i = 0; i < 20; ++i) {
                game.Tick(1000);
            }

            THEN("Only one item is in the bag") {
                CHECK(game.GetPlayers().at(pid).GetBag().size() == 1);
            }

            THEN("The second item remains on the map") {
                CHECK(game.GetLostObjects().Size() == 1);
            }
        }
    }
}

// =========================================================================
// 3. Сдача предметов на базу
// =========================================================================
SCENARIO("Player returns items to the base", "[find_return][return]") {
    GIVEN("A game with a base at (0, 10), the player starts right there") {
        model::Game game{1.0};
        // База совпадает со стартовой позицией игрока
        game.AddMap(MakeMapWithBase(2, 3, {0, 10}));

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(0, {1.0, 10.0});  // близкий предмет

        WHEN("Player moves right, picks up the item, then returns left to base") {
            game.SetPlayerDirection(pid, "R");
            for (int i = 0; i < 3; ++i) game.Tick(1000);
            REQUIRE(game.GetPlayers().at(pid).GetBag().size() == 1);

            game.SetPlayerDirection(pid, "L");
            for (int i = 0; i < 5; ++i) game.Tick(1000);

            THEN("Bag is empty after reaching the base") {
                CHECK(game.GetPlayers().at(pid).GetBag().empty());
            }
        }
    }
}

// =========================================================================
// 4. Один тик: игрок проходит мимо базы с полным рюкзаком и сдаёт предметы
// =========================================================================
SCENARIO("Bag capacity preserved between ticks", "[find_return][bag_full]") {
    GIVEN("A game with capacity 2 and two items") {
        model::Game game{1.0};
        game.AddMap(MakeMap(2, 2));

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(0, {3.0, 10.0});
        game.GetLostObjects().Add(0, {6.0, 10.0});

        WHEN("Player picks up both items over many ticks") {
            game.SetPlayerDirection(pid, "R");
            for (int i = 0; i < 20; ++i) game.Tick(1000);

            THEN("Both items are in the bag, in order of pickup") {
                const auto& bag = game.GetPlayers().at(pid).GetBag();
                REQUIRE(bag.size() == 2);
                CHECK(bag[0].id == 0);
                CHECK(bag[1].id == 1);
            }
        }
    }
}

// =========================================================================
// 5. Сдача и повторный сбор в одном тике
// =========================================================================
SCENARIO("Player returns items and picks up a new one in the same tick",
         "[find_return][mixed]") {
    GIVEN("A base in the middle of the road, items before and after it") {
        model::Game game{1.0};
        game.AddMap(MakeMapWithBase(2, 3, {10, 10}));

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(0, {5.0, 10.0});   // до базы
        game.GetLostObjects().Add(0, {15.0, 10.0});  // после базы

        WHEN("Player runs right across the base in one big tick") {
            game.SetPlayerDirection(pid, "R");
            // Один тик на 20 сек, игрок пройдёт всю дорогу от 0 до 20
            game.Tick(20000);

            THEN("Bag contains only the second item") {
                const auto& bag = game.GetPlayers().at(pid).GetBag();
                REQUIRE(bag.size() == 1);
                CHECK(bag[0].id == 1);  // второй по счёту id (0 и 1)
            }

            THEN("First item was returned, second was picked up") {
                // Первого предмета на карте нет, второго — тоже (он в рюкзаке)
                CHECK(game.GetLostObjects().Size() == 0);
            }
        }
    }
}

// =========================================================================
// 6. Несколько игроков и один предмет: побеждает тот, кто раньше
// =========================================================================
SCENARIO("The faster player picks up the item", "[find_return][two_players]") {
    GIVEN("Two players, item between them, both heading to it") {
        model::Game game{1.0};
        game.AddMap(MakeMap(2, 3));

        const auto p1 = game.AddPlayer("dog1", model::Map::Id{"m"});
        const auto p2 = game.AddPlayer("dog2", model::Map::Id{"m"});

        // Оба игрока стартуют в одной точке (0,10) — так как randomize_spawn off.
        // Разведём их направлениями: p1 идёт вправо, p2 тоже, но у p2 выше скорость.
        // В этой задаче скорость одна для карты, поэтому просто
        // убедимся, что предмет достаётся тому, у кого событие раньше.
        game.GetLostObjects().Add(0, {5.0, 10.0});

        WHEN("Both players move right") {
            game.SetPlayerDirection(p1, "R");
            game.SetPlayerDirection(p2, "R");
            game.Tick(6000);

            THEN("Exactly one of the bags contains the item") {
                bool in1 = !game.GetPlayers().at(p1).GetBag().empty();
                bool in2 = !game.GetPlayers().at(p2).GetBag().empty();
                CHECK((in1 ^ in2));  // ровно у одного
            }

            THEN("The item is no longer on the map") {
                CHECK(game.GetLostObjects().Size() == 0);
            }
        }
    }
}

// =========================================================================
// 7. Предмет в стороне от пути не подбирается
// =========================================================================
SCENARIO("Item off the path is not collected", "[find_return][miss]") {
    GIVEN("An item far from the road") {
        model::Game game{1.0};
        game.AddMap(MakeMap(2, 3));

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(0, {10.0, 10.0 + 5.0});  // далеко от дороги

        WHEN("Player moves right along the road") {
            game.SetPlayerDirection(pid, "R");
            for (int i = 0; i < 20; ++i) game.Tick(1000);

            THEN("Item stays on the map") {
                CHECK(game.GetLostObjects().Size() == 1);
                CHECK(game.GetPlayers().at(pid).GetBag().empty());
            }
        }
    }
}

// =========================================================================
// 8. Нулевое перемещение игрока — ничего не подбирается
// =========================================================================
SCENARIO("Stationary player does not collect items", "[find_return][idle]") {
    GIVEN("A player without movement and an item on the road") {
        model::Game game{1.0};
        game.AddMap(MakeMap(2, 3));

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(0, {0.0, 10.0});  // прямо на старте

        WHEN("Tick is called without setting direction") {
            game.Tick(1000);

            THEN("Item is not picked up because the player didn't move") {
                // Даже если игрок стоит в точке предмета — по условию
                // "если объект не переместился, считаем что столкновений не было"
                CHECK(game.GetLostObjects().Size() == 1);
                CHECK(game.GetPlayers().at(pid).GetBag().empty());
            }
        }
    }
}

// =========================================================================
// 9. Порядок предметов в рюкзаке = порядок сбора
// =========================================================================
SCENARIO("Bag preserves pickup order", "[find_return][order]") {
    GIVEN("Three items on the path in a specific order") {
        model::Game game{1.0};
        game.AddMap(MakeMap(2, 5));

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(10, {3.0, 10.0});
        game.GetLostObjects().Add(20, {6.0, 10.0});
        game.GetLostObjects().Add(30, {9.0, 10.0});

        WHEN("Player moves right across all items") {
            game.SetPlayerDirection(pid, "R");
            for (int i = 0; i < 20; ++i) game.Tick(1000);

            THEN("Items are in the bag in pickup order") {
                const auto& bag = game.GetPlayers().at(pid).GetBag();
                REQUIRE(bag.size() == 3);
                CHECK(bag[0].type == 10);
                CHECK(bag[1].type == 20);
                CHECK(bag[2].type == 30);
                // id в порядке добавления: 0, 1, 2
                CHECK(bag[0].id == 0);
                CHECK(bag[1].id == 1);
                CHECK(bag[2].id == 2);
            }
        }
    }
}

// =========================================================================
// 10. Предмет не подбирается дважды двумя игроками
// =========================================================================
SCENARIO("Same item cannot be picked up by two players", "[find_return][two_players]") {
    GIVEN("Two players and one item") {
        model::Game game{1.0};
        game.AddMap(MakeMap(2, 5));

        const auto p1 = game.AddPlayer("dog1", model::Map::Id{"m"});
        const auto p2 = game.AddPlayer("dog2", model::Map::Id{"m"});
        game.GetLostObjects().Add(0, {3.0, 10.0});

        WHEN("Both players pass through the item") {
            game.SetPlayerDirection(p1, "R");
            game.SetPlayerDirection(p2, "R");
            for (int i = 0; i < 20; ++i) game.Tick(1000);

            THEN("Only one bag has the item, total items collected = 1") {
                std::size_t total = game.GetPlayers().at(p1).GetBag().size()
                                  + game.GetPlayers().at(p2).GetBag().size();
                CHECK(total == 1);
                CHECK(game.GetLostObjects().Size() == 0);
            }
        }
    }
}

// =========================================================================
// 11. Сбор в тик, следующий за тиком, где игрок не двигался
// =========================================================================
SCENARIO("Player collects items only when moving", "[find_return][idle]") {
    GIVEN("Item far from start") {
        model::Game game{1.0};
        game.AddMap(MakeMap(2, 3));

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(0, {10.0, 10.0});

        WHEN("Player does several idle ticks, then starts moving") {
            for (int i = 0; i < 5; ++i) game.Tick(1000);  // стоит
            CHECK(game.GetPlayers().at(pid).GetBag().empty());

            game.SetPlayerDirection(pid, "R");
            for (int i = 0; i < 20; ++i) game.Tick(1000);

            THEN("Item is collected after movement begins") {
                CHECK(game.GetPlayers().at(pid).GetBag().size() == 1);
            }
        }
    }
}

// =========================================================================
// 12. Потерянный объект сохраняет свой type при попадании в рюкзак
// =========================================================================
SCENARIO("Lost object keeps its type in the bag", "[find_return][type]") {
    GIVEN("Item with type 7") {
        model::Game game{1.0};
        game.AddMap(MakeMap(10, 3));  // 10 типов трофеев

        const auto pid = game.AddPlayer("dog", model::Map::Id{"m"});
        game.GetLostObjects().Add(7, {5.0, 10.0});

        WHEN("Player picks up the item") {
            game.SetPlayerDirection(pid, "R");
            for (int i = 0; i < 10; ++i) game.Tick(1000);

            THEN("Type in the bag equals the original type") {
                const auto& bag = game.GetPlayers().at(pid).GetBag();
                REQUIRE(bag.size() == 1);
                CHECK(bag[0].type == 7);
            }
        }
    }
}
