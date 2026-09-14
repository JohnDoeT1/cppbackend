#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "collision_detector.h"
#include "geom.h"
#include "loot_generator.h"
#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

enum class Direction {
    NORTH = 0,
    SOUTH = 1,
    WEST = 2,
    EAST = 3
};

struct Speed {
    double vx = 0.0;
    double vy = 0.0;
};

struct Position {
    double x = 0.0;
    double y = 0.0;
};

// Потерянный объект
using LostObjectId = std::uint32_t;
using LootType = std::uint32_t;

struct LostObject {
    LostObjectId id = 0;
    LootType type = 0;
    Position pos;
};

class Road {
    struct HorizontalTag { explicit HorizontalTag() = default; };
    struct VerticalTag { explicit VerticalTag() = default; };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}, end_{end_x, start.y} {}
    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}, end_{start.x, end_y} {}

    bool IsHorizontal() const noexcept { return start_.y == end_.y; }
    bool IsVertical() const noexcept { return start_.x == end_.x; }
    Point GetStart() const noexcept { return start_; }
    Point GetEnd() const noexcept { return end_; }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept : bounds_{bounds} {}
    const Rectangle& GetBounds() const noexcept { return bounds_; }
private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}, position_{position}, offset_{offset} {}

    const Id& GetId() const noexcept { return id_; }
    Point GetPosition() const noexcept { return position_; }
    Offset GetOffset() const noexcept { return offset_; }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Dog {
public:
    Dog(std::string name, Position pos)
        : name_(std::move(name)), position_(pos) {}

    const std::string& GetName() const noexcept { return name_; }
    const Position& GetPosition() const noexcept { return position_; }
    void SetPosition(const Position& pos) noexcept { position_ = pos; }

    const Speed& GetSpeed() const noexcept { return speed_; }
    void SetSpeed(const Speed& speed) noexcept { speed_ = speed; }

    Direction GetDirection() const noexcept { return direction_; }
    void SetDirection(Direction dir) noexcept { direction_ = dir; }

private:
    std::string name_;
    Position position_;
    Speed speed_;
    Direction direction_ = Direction::NORTH;
};

class Player {
public:
    using Id = uint32_t;

    Player(Id id, std::string name, Position pos, std::size_t bag_capacity)
        : id_(id)
        , dog_(std::make_shared<Dog>(std::move(name), pos))
        , bag_capacity_(bag_capacity) {}

    Id GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return dog_->GetName(); }
    const std::shared_ptr<Dog>& GetDog() const noexcept { return dog_; }

    // --- рюкзак ---
    bool IsBagFull() const noexcept { return bag_.size() >= bag_capacity_; }
    void PutInBag(const LostObject& obj) { bag_.push_back(obj); }
    void ClearBag() { bag_.clear(); }
    const std::vector<LostObject>& GetBag() const noexcept { return bag_; }
    std::size_t GetBagCapacity() const noexcept { return bag_capacity_; }

private:
    Id id_;
    std::shared_ptr<Dog> dog_;
    std::vector<LostObject> bag_;
    std::size_t bag_capacity_ = 3;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name, double dog_speed,
        std::uint32_t loot_types_count, std::size_t bag_capacity) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_speed_(dog_speed)
        , loot_types_count_(loot_types_count)
        , bag_capacity_(bag_capacity) {}

    // Обратная совместимость: без bagCapacity — по умолчанию 3.
    Map(Id id, std::string name, double dog_speed,
        std::uint32_t loot_types_count) noexcept
        : Map(std::move(id), std::move(name), dog_speed, loot_types_count, 3) {}

    Map(Id id, std::string name, double dog_speed = 1.0) noexcept
        : Map(std::move(id), std::move(name), dog_speed, 0, 3) {}

    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const Buildings& GetBuildings() const noexcept { return buildings_; }
    const Roads& GetRoads() const noexcept { return roads_; }
    const Offices& GetOffices() const noexcept { return offices_; }
    double GetDogSpeed() const noexcept { return dog_speed_; }
    std::uint32_t GetLootTypesCount() const noexcept { return loot_types_count_; }
    std::size_t GetBagCapacity() const noexcept { return bag_capacity_; }

    void AddRoad(const Road& road) { roads_.emplace_back(road); }
    void AddBuilding(const Building& building) { buildings_.emplace_back(building); }
    void AddOffice(Office office);

    Position GetStartPositionOnRoad() const {
        if (roads_.empty()) return Position{0.0, 0.0};
        const Road& road = roads_[0];
        return Position{static_cast<double>(road.GetStart().x),
                        static_cast<double>(road.GetStart().y)};
    }

    Position GetRandomPositionOnRoad() const {
        if (roads_.empty()) return Position{0.0, 0.0};

        static std::random_device rd;
        static std::mt19937 gen(rd());

        std::uniform_int_distribution<> road_dist(0, static_cast<int>(roads_.size()) - 1);
        const Road& road = roads_[road_dist(gen)];

        const Point start = road.GetStart();
        const Point end = road.GetEnd();

        if (road.IsHorizontal()) {
            const double min_x = std::min(static_cast<double>(start.x),
                                          static_cast<double>(end.x));
            const double max_x = std::max(static_cast<double>(start.x),
                                          static_cast<double>(end.x));
            std::uniform_real_distribution<> dist(min_x, max_x);
            return Position{dist(gen), static_cast<double>(start.y)};
        }

        const double min_y = std::min(static_cast<double>(start.y),
                                      static_cast<double>(end.y));
        const double max_y = std::max(static_cast<double>(start.y),
                                      static_cast<double>(end.y));
        std::uniform_real_distribution<> dist(min_y, max_y);
        return Position{static_cast<double>(start.x), dist(gen)};
    }

    bool IsPositionOnRoad(const Position& pos) const {
        constexpr double HALF_WIDTH = 0.4;

        for (const auto& road : roads_) {
            const Point start = road.GetStart();
            const Point end = road.GetEnd();

            if (road.IsHorizontal()) {
                const double min_x = std::min(static_cast<double>(start.x),
                                              static_cast<double>(end.x));
                const double max_x = std::max(static_cast<double>(start.x),
                                              static_cast<double>(end.x));
                const double road_y = static_cast<double>(start.y);
                if (std::abs(pos.y - road_y) <= HALF_WIDTH &&
                    pos.x >= min_x - HALF_WIDTH && pos.x <= max_x + HALF_WIDTH) {
                    return true;
                }
            } else {
                const double min_y = std::min(static_cast<double>(start.y),
                                              static_cast<double>(end.y));
                const double max_y = std::max(static_cast<double>(start.y),
                                              static_cast<double>(end.y));
                const double road_x = static_cast<double>(start.x);
                if (std::abs(pos.x - road_x) <= HALF_WIDTH &&
                    pos.y >= min_y - HALF_WIDTH && pos.y <= max_y + HALF_WIDTH) {
                    return true;
                }
            }
        }
        return false;
    }

private:
    using OfficeIdToIndex =
        std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    double dog_speed_;
    std::uint32_t loot_types_count_ = 0;
    std::size_t bag_capacity_ = 3;
    Roads roads_;
    Buildings buildings_;
    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

// Хранилище потерянных объектов
class LostObjects {
public:
    using Storage = std::unordered_map<LostObjectId, LostObject>;

    LostObjectId Add(LootType type, Position pos) {
        LostObjectId id = next_id_++;
        LostObject obj{id, type, pos};
        objects_.emplace(id, std::move(obj));
        return id;
    }

    bool Remove(LostObjectId id) { return objects_.erase(id) > 0; }

    const LostObject* Find(LostObjectId id) const {
        auto it = objects_.find(id);
        return it == objects_.end() ? nullptr : &it->second;
    }

    const Storage& Get() const noexcept { return objects_; }
    std::size_t Size() const noexcept { return objects_.size(); }

private:
    LostObjectId next_id_ = 0;
    Storage objects_;
};

class Game {
public:
    using Maps = std::vector<Map>;
    using Players = std::unordered_map<uint32_t, Player>;
    using TokenToPlayerId = std::unordered_map<std::string, uint32_t>;
    using MapPlayers = std::unordered_map<Map::Id, std::vector<uint32_t>,
                                          util::TaggedHasher<Map::Id>>;

    Game(std::shared_ptr<loot_gen::LootGenerator> loot_gen, double default_dog_speed = 1.0)
        : loot_gen_{std::move(loot_gen)}, default_dog_speed_(default_dog_speed) {}

    Game(double default_dog_speed = 1.0) : Game(nullptr, default_dog_speed) {}

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept { return maps_; }

    const Map* FindMap(const Map::Id& id) const noexcept {
        auto it = map_id_to_index_.find(id);
        return it != map_id_to_index_.end() ? &maps_.at(it->second) : nullptr;
    }

    uint32_t AddPlayer(const std::string& name, const Map::Id& map_id) {
        uint32_t player_id = next_player_id_++;
        const Map* map = FindMap(map_id);
        Position pos;
        std::size_t cap = 3;
        if (map) {
            pos = randomize_spawn_points_ ? map->GetRandomPositionOnRoad()
                                          : map->GetStartPositionOnRoad();
            cap = map->GetBagCapacity();
        }
        Player player(player_id, name, pos, cap);
        players_.emplace(player_id, std::move(player));
        map_players_[map_id].push_back(player_id);
        return player_id;
    }

    const Players& GetPlayers() const noexcept { return players_; }

    const std::vector<uint32_t>& GetPlayersOnMap(const Map::Id& map_id) const {
        static const std::vector<uint32_t> empty;
        auto it = map_players_.find(map_id);
        return it == map_players_.end() ? empty : it->second;
    }

    bool HasPlayer(uint32_t player_id) const noexcept {
        return players_.contains(player_id);
    }

    void SetPlayerToken(const std::string& token, uint32_t player_id) {
        token_to_player_[token] = player_id;
        player_to_token_[player_id] = token;
    }

    std::optional<uint32_t> GetPlayerIdByToken(const std::string& token) const {
        auto it = token_to_player_.find(token);
        return it == token_to_player_.end() ? std::nullopt : std::optional{it->second};
    }

    model::Map::Id GetPlayerMap(uint32_t player_id) const {
        for (const auto& [map_id, players] : map_players_) {
            if (std::find(players.begin(), players.end(), player_id) != players.end()) {
                return map_id;
            }
        }
        throw std::runtime_error("Player not found on any map");
    }

    void SetRandomizeSpawnPoints(bool value) { randomize_spawn_points_ = value; }

    const Map* GetMapById(const Map::Id& map_id) const { return FindMap(map_id); }

    const LostObjects& GetLostObjects() const noexcept { return lost_objects_; }
    LostObjects& GetLostObjects() noexcept { return lost_objects_; }

    void SetPlayerDirection(uint32_t player_id, const std::string& direction) {
        auto it = players_.find(player_id);
        if (it == players_.end()) return;

        auto& dog = it->second.GetDog();
        model::Map::Id map_id = GetPlayerMap(player_id);
        const Map* map = FindMap(map_id);
        if (!map) return;

        double speed = map->GetDogSpeed();
        Speed new_speed{0.0, 0.0};
        Direction new_direction = dog->GetDirection();

        if (direction == "L") { new_speed = Speed{-speed, 0.0}; new_direction = Direction::WEST; }
        else if (direction == "R") { new_speed = Speed{speed, 0.0}; new_direction = Direction::EAST; }
        else if (direction == "U") { new_speed = Speed{0.0, -speed}; new_direction = Direction::NORTH; }
        else if (direction == "D") { new_speed = Speed{0.0, speed}; new_direction = Direction::SOUTH; }
        else if (direction == "") { new_speed = Speed{0.0, 0.0}; }

        dog->SetSpeed(new_speed);
        if (direction != "") dog->SetDirection(new_direction);
    }

    // ---------- Tick ----------
    void Tick(int time_delta_ms) {
        constexpr double HALF_WIDTH = 0.4;
        constexpr double EPSILON = 1e-9;
        const double dt = time_delta_ms / 1000.0;

        // --- 1. Запоминаем старт ---
        std::unordered_map<uint32_t, Position> start_positions;
        start_positions.reserve(players_.size());
        for (auto& [id, player] : players_) {
            start_positions[id] = player.GetDog()->GetPosition();
        }

        // --- 2. Движение собак (без изменений) ---
        for (auto& [player_id, player] : players_) {
            auto& dog = player.GetDog();
            const Speed speed = dog->GetSpeed();
            if (std::abs(speed.vx) < EPSILON && std::abs(speed.vy) < EPSILON) continue;

            const Map::Id map_id = GetPlayerMap(player_id);
            const Map* map = FindMap(map_id);
            if (!map) continue;

            const Position pos = dog->GetPosition();
            const Position target{pos.x + speed.vx * dt, pos.y + speed.vy * dt};

            if (std::abs(speed.vx) > EPSILON) {
                double min_x = pos.x, max_x = pos.x;
                bool changed = true;
                while (changed) {
                    changed = false;
                    for (const auto& road : map->GetRoads()) {
                        double road_min, road_max;
                        if (road.IsHorizontal()) {
                            const double road_y = static_cast<double>(road.GetStart().y);
                            if (std::abs(pos.y - road_y) > HALF_WIDTH) continue;
                            road_min = std::min(static_cast<double>(road.GetStart().x),
                                                static_cast<double>(road.GetEnd().x)) - HALF_WIDTH;
                            road_max = std::max(static_cast<double>(road.GetStart().x),
                                                static_cast<double>(road.GetEnd().x)) + HALF_WIDTH;
                        } else {
                            const double road_x = static_cast<double>(road.GetStart().x);
                            const double road_min_y = std::min(static_cast<double>(road.GetStart().y),
                                                                static_cast<double>(road.GetEnd().y)) - HALF_WIDTH;
                            const double road_max_y = std::max(static_cast<double>(road.GetStart().y),
                                                                static_cast<double>(road.GetEnd().y)) + HALF_WIDTH;
                            if (pos.y < road_min_y || pos.y > road_max_y) continue;
                            road_min = road_x - HALF_WIDTH;
                            road_max = road_x + HALF_WIDTH;
                        }
                        if (road_max >= min_x && road_min <= max_x) {
                            if (road_min < min_x) { min_x = road_min; changed = true; }
                            if (road_max > max_x) { max_x = road_max; changed = true; }
                        }
                    }
                }
                Position new_pos = target;
                if (speed.vx > 0) { if (target.x > max_x) { new_pos.x = max_x; dog->SetSpeed(Speed{0.0, 0.0}); } }
                else { if (target.x < min_x) { new_pos.x = min_x; dog->SetSpeed(Speed{0.0, 0.0}); } }
                dog->SetPosition(new_pos);
                continue;
            }

            if (std::abs(speed.vy) > EPSILON) {
                double min_y = pos.y, max_y = pos.y;
                bool changed = true;
                while (changed) {
                    changed = false;
                    for (const auto& road : map->GetRoads()) {
                        double road_min, road_max;
                        if (road.IsVertical()) {
                            const double road_x = static_cast<double>(road.GetStart().x);
                            if (std::abs(pos.x - road_x) > HALF_WIDTH) continue;
                            road_min = std::min(static_cast<double>(road.GetStart().y),
                                                static_cast<double>(road.GetEnd().y)) - HALF_WIDTH;
                            road_max = std::max(static_cast<double>(road.GetStart().y),
                                                static_cast<double>(road.GetEnd().y)) + HALF_WIDTH;
                        } else {
                            const double road_y = static_cast<double>(road.GetStart().y);
                            const double road_min_x = std::min(static_cast<double>(road.GetStart().x),
                                                                static_cast<double>(road.GetEnd().x)) - HALF_WIDTH;
                            const double road_max_x = std::max(static_cast<double>(road.GetStart().x),
                                                                static_cast<double>(road.GetEnd().x)) + HALF_WIDTH;
                            if (pos.x < road_min_x || pos.x > road_max_x) continue;
                            road_min = road_y - HALF_WIDTH;
                            road_max = road_y + HALF_WIDTH;
                        }
                        if (road_max >= min_y && road_min <= max_y) {
                            if (road_min < min_y) { min_y = road_min; changed = true; }
                            if (road_max > max_y) { max_y = road_max; changed = true; }
                        }
                    }
                }
                Position new_pos = target;
                if (speed.vy > 0) { if (target.y > max_y) { new_pos.y = max_y; dog->SetSpeed(Speed{0.0, 0.0}); } }
                else { if (target.y < min_y) { new_pos.y = min_y; dog->SetSpeed(Speed{0.0, 0.0}); } }
                dog->SetPosition(new_pos);
            }
        }

        // --- 3. Сбор и сдача ---
        HandleGatherAndReturn(start_positions);

        // --- 4. Генерация новых потеряшек (без изменений) ---
        if (!loot_gen_) return;

        const auto time_delta = std::chrono::milliseconds(time_delta_ms);
        const unsigned looters = static_cast<unsigned>(players_.size());
        const unsigned loot = static_cast<unsigned>(lost_objects_.Size());
        const unsigned to_generate = loot_gen_->Generate(time_delta, loot, looters);

        if (to_generate == 0 || maps_.empty()) return;

        const Map& map = maps_.front();
        for (unsigned i = 0; i < to_generate; ++i) {
            LootType type = 0;
            if (map.GetLootTypesCount() > 0) {
                std::uniform_int_distribution<std::uint32_t> type_dist{
                    0, map.GetLootTypesCount() - 1};
                type = type_dist(gen_);
            }
            lost_objects_.Add(type, map.GetRandomPositionOnRoad());
        }
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    // Провайдер для FindGatherEvents: items + gatherers
    class Provider : public collision_detector::ItemGathererProvider {
    public:
        void AddItem(collision_detector::Item item) { items_.push_back(item); }
        void AddGatherer(collision_detector::Gatherer g) { gatherers_.push_back(g); }

        size_t ItemsCount() const override { return items_.size(); }
        collision_detector::Item GetItem(size_t idx) const override { return items_.at(idx); }
        size_t GatherersCount() const override { return gatherers_.size(); }
        collision_detector::Gatherer GetGatherer(size_t idx) const override {
            return gatherers_.at(idx);
        }

    private:
        std::vector<collision_detector::Item> items_;
        std::vector<collision_detector::Gatherer> gatherers_;
    };

    // Раздельные события: сбор предмета и сдача на базу
    struct TickEvent {
        enum class Type { Collect, Return };
        double time;
        Type type;
        uint32_t player_id;
        LostObjectId item_id = 0;  // используется только для Collect
    };

    void HandleGatherAndReturn(
        const std::unordered_map<uint32_t, Position>& start_positions) {

        if (maps_.empty() || players_.empty()) return;

        const Map& map = maps_.front();

        // Строим провайдера: gatherers — только игроки, items — потеряшки.
        Provider provider;

        // Items: потеряшки (полу-ширина = 0.0)
        // Запоминаем соответствие item_idx -> LostObjectId
        std::vector<LostObjectId> item_ids;
        item_ids.reserve(lost_objects_.Size());
        for (const auto& [id, obj] : lost_objects_.Get()) {
            collision_detector::Item it;
            it.position = geom::Point2D{obj.pos.x, obj.pos.y};
            it.width = 0.0;
            provider.AddItem(it);
            item_ids.push_back(id);
        }

        // Gatherers: игроки (полу-ширина = 0.3)
        std::vector<uint32_t> player_ids;
        player_ids.reserve(players_.size());
        for (const auto& [id, player] : players_) {
            const Position s = start_positions.at(id);
            const Position e = player.GetDog()->GetPosition();
            collision_detector::Gatherer g;
            g.start_pos = geom::Point2D{s.x, s.y};
            g.end_pos = geom::Point2D{e.x, e.y};
            g.width = 0.3;
            provider.AddGatherer(g);
            player_ids.push_back(id);
        }

        std::vector<TickEvent> events;

        // События сбора
        if (!player_ids.empty() && !item_ids.empty()) {
            auto collects = collision_detector::FindGatherEvents(provider);
            for (const auto& ev : collects) {
                TickEvent e;
                e.time = ev.time;
                e.type = TickEvent::Type::Collect;
                e.player_id = player_ids[ev.gatherer_id];
                e.item_id = item_ids[ev.item_id];
                events.push_back(e);
            }
        }

        // События сдачи: игрок дошёл до базы
        // Полу-ширина базы 0.25, полу-ширина игрока 0.3 → радиус 0.55
        for (const auto& office : map.GetOffices()) {
            const geom::Point2D office_pos{
                static_cast<double>(office.GetPosition().x),
                static_cast<double>(office.GetPosition().y)
            };
            for (std::size_t i = 0; i < player_ids.size(); ++i) {
                const Position s = start_positions.at(player_ids[i]);
                const Position e = players_.at(player_ids[i]).GetDog()->GetPosition();
                const geom::Point2D a{s.x, s.y};
                const geom::Point2D b{e.x, e.y};

                if (std::abs(b.x - a.x) < 1e-12 && std::abs(b.y - a.y) < 1e-12) {
                    continue;  // игрок не двигался
                }

                auto res = collision_detector::TryCollectPoint(a, b, office_pos);
                constexpr double PLAYER_HALF = 0.3;
                constexpr double OFFICE_HALF = 0.25;
                if (res.IsCollected(PLAYER_HALF + OFFICE_HALF)) {
                    TickEvent e;
                    e.time = res.proj_ratio;
                    e.type = TickEvent::Type::Return;
                    e.player_id = player_ids[i];
                    events.push_back(e);
                }
            }
        }

        // Сортируем все события по времени
        std::sort(events.begin(), events.end(),
                  [](const TickEvent& a, const TickEvent& b) {
                      return a.time < b.time;
                  });

        // Обрабатываем
        for (const auto& ev : events) {
            auto pit = players_.find(ev.player_id);
            if (pit == players_.end()) continue;
            Player& player = pit->second;

            if (ev.type == TickEvent::Type::Collect) {
                if (player.IsBagFull()) continue;
                const LostObject* obj = lost_objects_.Find(ev.item_id);
                if (!obj) continue;  // уже кто-то взял
                player.PutInBag(*obj);
                lost_objects_.Remove(ev.item_id);
            } else {  // Return
                player.ClearBag();
            }
        }
    }

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    Players players_;
    TokenToPlayerId token_to_player_;
    std::unordered_map<uint32_t, std::string> player_to_token_;
    MapPlayers map_players_;
    uint32_t next_player_id_ = 0;
    double default_dog_speed_;
    bool randomize_spawn_points_ = false;

    std::shared_ptr<loot_gen::LootGenerator> loot_gen_;
    LostObjects lost_objects_;
    std::mt19937 gen_{std::random_device{}()};
};

}  // namespace model
