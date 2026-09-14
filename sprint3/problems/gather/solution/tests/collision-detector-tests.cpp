#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include "../src/collision_detector.h"

#include <sstream>
#include <vector>

using namespace collision_detector;
using Catch::Matchers::WithinAbs;

// --- Печать GatheringEvent в Catch2 ---
namespace Catch {
template <>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id << ","
            << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};
}  // namespace Catch

namespace {

constexpr double EPS = 1e-10;

// Простейший провайдер: векторы предметов и собирателей.
class SimpleProvider : public ItemGathererProvider {
public:
    void AddItem(geom::Point2D pos, double w) {
        items_.push_back(Item{pos, w});
    }
    void AddGatherer(geom::Point2D start, geom::Point2D end, double w) {
        gatherers_.push_back(Gatherer{start, end, w});
    }

    size_t ItemsCount() const override { return items_.size(); }
    Item GetItem(size_t idx) const override { return items_.at(idx); }

    size_t GatherersCount() const override { return gatherers_.size(); }
    Gatherer GetGatherer(size_t idx) const override { return gatherers_.at(idx); }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

// Сравнение событий с погрешностью.
bool EventsEqual(const GatheringEvent& a, const GatheringEvent& b) {
    return a.item_id == b.item_id && a.gatherer_id == b.gatherer_id &&
           std::abs(a.sq_distance - b.sq_distance) < EPS &&
           std::abs(a.time - b.time) < EPS;
}

}  // namespace

// =========================================================================
// 1. Базовый сбор: предмет точно на пути, попадает в радиус
// =========================================================================
TEST_CASE("Item directly on the path is collected", "[FindGatherEvents]") {
    SimpleProvider p;
    // Собака: старт (0,0), конец (10,0), ширина 1.0
    p.AddGatherer({0, 0}, {10, 0}, 1.0);
    // Предмет: (5, 0), ширина 1.0
    p.AddItem({5, 0}, 1.0);

    auto events = FindGatherEvents(p);
    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK_THAT(events[0].sq_distance, WithinAbs(0.0, EPS));
    CHECK_THAT(events[0].time, WithinAbs(0.5, EPS));
}

// =========================================================================
// 2. Сбор «касанием»: ловит Wrong1 и Wrong2
// =========================================================================
TEST_CASE("Item is collected when it touches only the gatherer edge",
          "[FindGatherEvents][wrong1][wrong2]") {
    // gatherer.width + item.width = 3.0 — это радиус сбора.
    // Ставим предмет так, чтобы расстояние до оси было 2.5:
    //   правильная: 2.5 <= 3.0 -> собирает
    //   Wrong1 (2.0): 2.5 > 2.0 -> не собирает (тест упадёт)
    //   Wrong2 (1.0): 2.5 > 1.0 -> не собирает (тест упадёт)
    SimpleProvider p;
    p.AddGatherer({0, 0}, {10, 0}, 2.0);
    p.AddItem({5, 2.5}, 1.0);

    auto events = FindGatherEvents(p);
    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK_THAT(events[0].time, WithinAbs(0.5, EPS));
    CHECK_THAT(events[0].sq_distance, WithinAbs(2.5 * 2.5, EPS));
}

// =========================================================================
// 3. Порядок событий по времени — ловит Wrong3 (нет сортировки)
//    Первый по индексам собиратель ловит свой предмет ПОЗЖЕ второго.
// =========================================================================
TEST_CASE("Events are sorted by time", "[FindGatherEvents][wrong3]") {
    SimpleProvider p;
    // gatherer 0: (0,0) -> (10,0), ловит предмет на time=0.9
    p.AddGatherer({0, 0}, {10, 0}, 1.0);
    // gatherer 1: (0,0) -> (10,0), ловит предмет на time=0.1
    p.AddGatherer({0, 0}, {10, 0}, 1.0);

    // item 0: на (9, 0), time относительно gatherer 0 = 0.9
    p.AddItem({9, 0}, 1.0);
    // item 1: на (1, 0), time относительно gatherer 1 = 0.1
    p.AddItem({1, 0}, 1.0);

    auto events = FindGatherEvents(p);
    REQUIRE(events.size() == 4);  // каждый собиратель ловит оба предмета

    // Проверяем, что time монотонно не убывает
    for (size_t i = 1; i < events.size(); ++i) {
        CHECK(events[i - 1].time <= events[i].time + EPS);
    }

    // Первое событие должно иметь time == 0.1 (item 1 от gatherer 1)
    // В неправильной реализации (без sort) порядок был бы: (g0,i0), (g0,i1), (g1,i0), (g1,i1)
    // и первое событие имело бы time == 0.9.
    CHECK_THAT(events.front().time, WithinAbs(0.1, EPS));
    CHECK_THAT(events.back().time, WithinAbs(0.9, EPS));
}

// =========================================================================
// 4. Диагональное движение — ловит Wrong4 и Wrong5
//    (TryCollectPoint_Wrong1/2 дают неверные sq_distance/proj_ratio)
// =========================================================================
TEST_CASE("Diagonal movement collects item on the path",
          "[FindGatherEvents][wrong4][wrong5]") {
    SimpleProvider p;
    // Собака двигается по диагонали из (0,0) в (10,10), ширина 1.0
    p.AddGatherer({0, 0}, {10, 10}, 1.0);
    // Предмет точно по центру диагонали
    p.AddItem({5, 5}, 1.0);

    auto events = FindGatherEvents(p);
    REQUIRE(events.size() == 1);
    CHECK_THAT(events[0].sq_distance, WithinAbs(0.0, EPS));
    CHECK_THAT(events[0].time, WithinAbs(0.5, EPS));

    // А теперь сместим старт — Wrong1/Wrong2 не вычитают start, поэтому промахнутся
    SimpleProvider p2;
    p2.AddGatherer({100, 100}, {110, 110}, 1.0);
    p2.AddItem({105, 105}, 1.0);

    auto events2 = FindGatherEvents(p2);
    REQUIRE(events2.size() == 1);
    CHECK_THAT(events2[0].sq_distance, WithinAbs(0.0, EPS));
    CHECK_THAT(events2[0].time, WithinAbs(0.5, EPS));
}

// =========================================================================
// 5. Предмет вне проекции — не собирается
// =========================================================================
TEST_CASE("Item outside projection is not collected", "[FindGatherEvents]") {
    SimpleProvider p;
    p.AddGatherer({0, 0}, {10, 0}, 1.0);
    // Предмет «до» старта: proj_ratio < 0
    p.AddItem({-3, 0}, 1.0);
    // Предмет «после» конца: proj_ratio > 1
    p.AddItem({15, 0}, 1.0);
    // Предмет далеко в стороне: proj_ratio в диапазоне, но расстояние слишком большое
    p.AddItem({5, 10}, 1.0);

    CHECK(FindGatherEvents(p).empty());
}

// =========================================================================
// 6. Нулевое перемещение — событий быть не должно
// =========================================================================
TEST_CASE("Stationary gatherer has no events", "[FindGatherEvents]") {
    SimpleProvider p;
    p.AddGatherer({5, 5}, {5, 5}, 1.0);
    p.AddItem({5, 5}, 1.0);
    CHECK(FindGatherEvents(p).empty());
}

// =========================================================================
// 7. Граничные значения радиуса сбора
// =========================================================================
TEST_CASE("Boundary of collect radius", "[FindGatherEvents]") {
    // collect_radius = gatherer.width + item.width = 1.0 + 1.0 = 2.0.
    // Точка точно на границе:
    SimpleProvider p;
    p.AddGatherer({0, 0}, {10, 0}, 1.0);
    p.AddItem({5, 2.0}, 1.0);
    CHECK(FindGatherEvents(p).size() == 1);

    // Чуть-чуть за границей:
    SimpleProvider p2;
    p2.AddGatherer({0, 0}, {10, 0}, 1.0);
    p2.AddItem({5, 2.0000001}, 1.0);
    CHECK(FindGatherEvents(p2).empty());
}

// =========================================================================
// 8. Несколько предметов у одного собирателя, порядок по времени
// =========================================================================
TEST_CASE("Multiple items ordered by time", "[FindGatherEvents]") {
    SimpleProvider p;
    p.AddGatherer({0, 0}, {10, 0}, 1.0);
    p.AddItem({8, 0}, 1.0);  // time = 0.8
    p.AddItem({2, 0}, 1.0);  // time = 0.2
    p.AddItem({5, 0}, 1.0);  // time = 0.5

    auto events = FindGatherEvents(p);
    REQUIRE(events.size() == 3);
    CHECK_THAT(events[0].time, WithinAbs(0.2, EPS));
    CHECK_THAT(events[1].time, WithinAbs(0.5, EPS));
    CHECK_THAT(events[2].time, WithinAbs(0.8, EPS));

    // Проверяем, что sq_distance = 0 у всех (предметы точно на оси)
    for (const auto& e : events) {
        CHECK_THAT(e.sq_distance, WithinAbs(0.0, EPS));
    }
}

// =========================================================================
// 9. Один и тот же предмет собирается несколькими собирателями —
//    событие добавляется для каждого.
// =========================================================================
TEST_CASE("Same item collected by multiple gatherers", "[FindGatherEvents]") {
    SimpleProvider p;
    p.AddGatherer({0, 0}, {10, 0}, 1.0);
    p.AddGatherer({10, 0}, {0, 0}, 1.0);
    p.AddItem({5, 0}, 1.0);

    auto events = FindGatherEvents(p);
    REQUIRE(events.size() == 2);

    // Проверяем, что оба gatherer_id представлены
    bool has_g0 = false, has_g1 = false;
    for (const auto& e : events) {
        if (e.gatherer_id == 0) has_g0 = true;
        if (e.gatherer_id == 1) has_g1 = true;
        CHECK(e.item_id == 0);
    }
    CHECK(has_g0);
    CHECK(has_g1);
}

// =========================================================================
// 10. Проверка корректности sq_distance и time на диагонали
//     (используется и для отлова Wrong4, Wrong5)
// =========================================================================
TEST_CASE("Correct sq_distance and time on diagonal with offset start",
          "[FindGatherEvents][wrong4][wrong5]") {
    SimpleProvider p;
    // Собака: (3, 3) -> (13, 13), ширина 1.0
    p.AddGatherer({3, 3}, {13, 13}, 1.0);
    // Предмет: (5, 5), ширина 1.0
    // Ожидаем: sq_distance = 0 (на оси), time = (2*sqrt(2)) / (10*sqrt(2)) = 0.2
    p.AddItem({5, 5}, 1.0);

    auto events = FindGatherEvents(p);
    REQUIRE(events.size() == 1);
    CHECK_THAT(events[0].sq_distance, WithinAbs(0.0, EPS));
    CHECK_THAT(events[0].time, WithinAbs(0.2, EPS));
}
