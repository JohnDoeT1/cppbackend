#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
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

// ----- Потерянный объект на карте -----
using LostObjectId = std::uint32_t;
using LootType = std::uint32_t;
using Score = unsigned;

struct LostObject {
    LostObjectId id = 0;
    LootType type = 0;
    int value = 0;
    geom::Point2D pos;
};

// ----- Найденный объект (в рюкзаке собаки) -----
struct FoundObject {
    using Id = util::Tagged<std::uint32_t, FoundObject>;
    Id id{0u};
    LootType type{0u};

    [[nodiscard]] auto operator<=>(const FoundObject&) const = default;
};

// ----- Дороги, здания, офисы -----
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

// ============================================================
// Dog — с Id, рюкзаком FoundObject и счётом
// ============================================================
class Dog {
public:
    using Id = util::Tagged<std::uint32_t, Dog>;
    using BagContent = std::vector<FoundObject>;

    Dog() = default;

    Dog(Id id, std::string name, geom::Point2D pos, std::size_t bag_cap)
        : id_(std::move(id))
        , name_(std::move(name))
        , position_(pos)
        , bag_cap_(bag_cap) {
        bag_.reserve(bag_cap);
    }

    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const geom::Point2D& GetPosition() const noexcept { return position_; }
    void SetPosition(geom::Point2D pos) noexcept { position_ = pos; }

    const geom::Vec2D& GetSpeed() const noexcept { return speed_; }
    void SetSpeed(geom::Vec2D speed) noexcept { speed_ = speed; }

    Direction GetDirection() const noexcept { return direction_; }
    void SetDirection(Direction dir) noexcept { direction_ = dir; }

    std::size_t GetBagCapacity() const noexcept { return bag_cap_; }
    const BagContent& GetBagContent() const noexcept { return bag_; }

    bool IsBagFull() const noexcept { return bag_.size() >= bag_cap_; }

    bool PutToBag(FoundObject item) {
        if (IsBagFull()) return false;
        bag_.push_back(item);
        return true;
    }

    std::size_t EmptyBag() noexcept {
        auto n = bag_.size();
        bag_.clear();
        return n;
    }

    Score GetScore() const noexcept { return score_; }
    void AddScore(Score s) noexcept { score_ += s; }

private:
    Id id_{0u};
    std::string name_;
    geom::Point2D position_;
    geom::Vec2D speed_;
    Direction direction_ = Direction::NORTH;
    BagContent bag_;
    std::size_t bag_cap_ = 3;
    Score score_ = 0;
};

using DogPtr = std::shared_ptr<Dog>;
using ConstDogPtr = std::shared_ptr<const Dog>;

// ============================================================
// <-- ДОБАВЛЕНО: интерфейс слушателя ухода на пенсию
// ============================================================
class RetirementListener {
public:
    virtual void OnPlayerRetired(const std::string& name,
                                 int score,
                                 double play_time_sec) = 0;
    virtual ~RetirementListener() = default;
};

// ============================================================
// Player — обёртка над Dog
// ============================================================
class Player {
public:
    using Id = std::uint32_t;

    Player() = default;

    Player(Id id, std::string name, geom::Point2D pos, std::size_t bag_cap)
        : id_(id)
        , dog_(std::make_shared<Dog>(Dog::Id{id}, std::move(name), pos, bag_cap)) {}

    Player(Id id, DogPtr dog) : id_(id), dog_(std::move(dog)) {}

    Id GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return dog_->GetName(); }
    const DogPtr& GetDog() const noexcept { return dog_; }

    bool IsBagFull() const noexcept { return dog_->IsBagFull(); }
    const std::vector<FoundObject>& GetBag() const noexcept { return dog_->GetBagContent(); }
    std::size_t GetBagCapacity() const noexcept { return dog_->GetBagCapacity(); }
    int GetScore() const noexcept { return static_cast<int>(dog_->GetScore()); }

    // <-- ДОБАВЛЕНО: времена активности
    int64_t GetJoinTimeMs() const noexcept { return join_time_ms_; }
    void SetJoinTimeMs(int64_t v) noexcept { join_time_ms_ = v; }

    int64_t GetLastActiveTimeMs() const noexcept { return last_active_time_ms_; }
    void SetLastActiveTimeMs(int64_t v) noexcept { last_active_time_ms_ = v; }

private:
    Id id_ = 0;
    DogPtr dog_;
    int64_t join_time_ms_ = 0;          // <-- ДОБАВЛЕНО
    int64_t last_active_time_ms_ = 0;   // <-- ДОБАВЛЕНО
};

// ============================================================
// Map
// ============================================================
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

    geom::Point2D GetStartPositionOnRoad() const {
        if (roads_.empty()) return geom::Point2D{0.0, 0.0};
        const Road& road = roads_[0];
        return geom::Point2D{static_cast<double>(road.GetStart().x),
                             static_cast<double>(road.GetStart().y)};
    }

    geom::Point2D GetRandomPositionOnRoad() const {
        if (roads_.empty()) return geom::Point2D{0.0, 0.0};

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
            return geom::Point2D{dist(gen), static_cast<double>(start.y)};
        }

        const double min_y = std::min(static_cast<double>(start.y),
                                      static_cast<double>(end.y));
        const double max_y = std::max(static_cast<double>(start.y),
                                      static_cast<double>(end.y));
        std::uniform_real_distribution<> dist(min_y, max_y);
        return geom::Point2D{static_cast<double>(start.x), dist(gen)};
    }

    bool IsPositionOnRoad(const geom::Point2D& pos) const {
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
                    pos.x >= min_x - HALF_WIDTH && pos.x <= max_x + HALF_WIDTH) return true;
            } else {
                const double min_y = std::min(static_cast<double>(start.y),
                                              static_cast<double>(end.y));
                const double max_y = std::max(static_cast<double>(start.y),
                                              static_cast<double>(end.y));
                const double road_x = static_cast<double>(start.x);
                if (std::abs(pos.x - road_x) <= HALF_WIDTH &&
                    pos.y >= min_y - HALF_WIDTH && pos.y <= max_y + HALF_WIDTH) return true;
            }
        }
        return false;
    }

    void SetLootValues(std::vector<int> values) { loot_values_ = std::move(values); }
    int GetLootValue(LootType type) const {
        if (type < loot_values_.size()) return loot_values_[type];
        return 0;
    }

private:
    using OfficeIdToIndex =
        std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    double dog_speed_;
    std::uint32_t loot_types_count_ = 0;
    std::size_t bag_capacity_ = 3;
    std::vector<int> loot_values_;
    Roads roads_;
    Buildings buildings_;
    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

// ============================================================
// LostObjects
// ============================================================
class LostObjects {
public:
    using Storage = std::unordered_map<LostObjectId, LostObject>;

    LostObjectId Add(LootType type, int value, geom::Point2D pos) {
        LostObjectId id = next_id_++;
        LostObject obj{id, type, value, pos};
        objects_.emplace(id, std::move(obj));
        return id;
    }

    LostObjectId Add(LootType type, geom::Point2D pos) { return Add(type, 0, pos); }

    bool Remove(LostObjectId id) { return objects_.erase(id) > 0; }

    const LostObject* Find(LostObjectId id) const {
        auto it = objects_.find(id);
        return it == objects_.end() ? nullptr : &it->second;
    }

    const Storage& Get() const noexcept { return objects_; }
    std::size_t Size() const noexcept { return objects_.size(); }

    LostObjectId GetNextId() const noexcept { return next_id_; }
    void SetNextId(LostObjectId id) noexcept { next_id_ = id; }

    void SetStorage(Storage storage, LostObjectId next_id) {
        objects_ = std::move(storage);
        next_id_ = next_id;
    }

private:
    LostObjectId next_id_ = 0;
    Storage objects_;
};

// ============================================================
// Game
// ============================================================
class Game {
public:
    using Maps = std::vector<Map>;
    using Players = std::unordered_map<uint32_t, Player>;
    using TokenToPlayerId = std::unordered_map<std::string, uint32_t>;
    using MapPlayers = std::unordered_map<Map::Id, std::vector<uint32_t>,
                                          util::TaggedHasher<Map::Id>>;
    using TickListener = std::function<void(std::chrono::milliseconds)>;

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
        geom::Point2D pos{0.0, 0.0};
        std::size_t cap = 3;
        if (map) {
            pos = randomize_spawn_points_ ? map->GetRandomPositionOnRoad()
                                          : map->GetStartPositionOnRoad();
            cap = map->GetBagCapacity();
        }
        Player player(player_id, name, pos, cap);
        player.SetJoinTimeMs(elapsed_ms_);            // <-- ДОБАВЛЕНО
        player.SetLastActiveTimeMs(elapsed_ms_);      // <-- ДОБАВЛЕНО
        players_.emplace(player_id, std::move(player));
        map_players_[map_id].push_back(player_id);
        return player_id;
    }

    const Players& GetPlayers() const noexcept { return players_; }
    Players& GetPlayers() noexcept { return players_; }
    void SetPlayers(Players p) { players_ = std::move(p); }

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

    // ---- Слушатель и время ----
    void SetTickListener(TickListener listener) { tick_listener_ = std::move(listener); }
    int64_t GetElapsedMs() const noexcept { return elapsed_ms_; }
    void SetElapsedMs(int64_t v) noexcept { elapsed_ms_ = v; }

    // <-- ДОБАВЛЕНО: retirement
    void SetDogRetirementTime(int64_t ms) noexcept { dog_retirement_ms_ = ms; }
    int64_t GetDogRetirementTime() const noexcept { return dog_retirement_ms_; }

    void SetRetirementListener(RetirementListener* listener) noexcept {
        retirement_listener_ = listener;
    }

    // ============================================================
    // Аксессоры для сериализации
    // ============================================================

    // --- non-const (для Restore) ---
    TokenToPlayerId& GetTokenToPlayer() noexcept { return token_to_player_; }
    MapPlayers& GetMapPlayers() noexcept { return map_players_; }
    std::unordered_map<uint32_t, std::string>& GetPlayerToToken() noexcept {
        return player_to_token_;
    }

    // --- const (для Serialize) ---
    const TokenToPlayerId& GetTokenToPlayer() const noexcept { return token_to_player_; }
    const MapPlayers& GetMapPlayers() const noexcept { return map_players_; }
    const std::unordered_map<uint32_t, std::string>& GetPlayerToToken() const noexcept {
        return player_to_token_;
    }

    // --- сеттеры для Restore ---
    void SetTokenToPlayer(TokenToPlayerId t) { token_to_player_ = std::move(t); }
    void SetPlayerToToken(std::unordered_map<uint32_t, std::string> t) {
        player_to_token_ = std::move(t);
    }
    void SetMapPlayers(MapPlayers m) { map_players_ = std::move(m); }
    uint32_t GetNextPlayerId() const noexcept { return next_player_id_; }
    void SetNextPlayerId(uint32_t v) noexcept { next_player_id_ = v; }

    void SetPlayerDirection(uint32_t player_id, const std::string& direction) {
        auto it = players_.find(player_id);
        if (it == players_.end()) return;
        auto& dog = it->second.GetDog();
        model::Map::Id map_id = GetPlayerMap(player_id);
        const Map* map = FindMap(map_id);
        if (!map) return;

        double speed = map->GetDogSpeed();
        geom::Vec2D new_speed{0.0, 0.0};
        Direction new_direction = dog->GetDirection();

        if (direction == "L") { new_speed = geom::Vec2D{-speed, 0.0}; new_direction = Direction::WEST; }
        else if (direction == "R") { new_speed = geom::Vec2D{speed, 0.0}; new_direction = Direction::EAST; }
        else if (direction == "U") { new_speed = geom::Vec2D{0.0, -speed}; new_direction = Direction::NORTH; }
        else if (direction == "D") { new_speed = geom::Vec2D{0.0, speed}; new_direction = Direction::SOUTH; }
        else if (direction == "") { new_speed = geom::Vec2D{0.0, 0.0}; }

        dog->SetSpeed(new_speed);
        if (direction != "") {
            dog->SetDirection(new_direction);
            it->second.SetLastActiveTimeMs(elapsed_ms_);   // <-- ДОБАВЛЕНО
        }
    }

    void Tick(int time_delta_ms) {
        constexpr double HALF_WIDTH = 0.4;
        constexpr double EPSILON = 1e-9;
        const double dt = time_delta_ms / 1000.0;

        // инкрементируем elapsed_ms_ до обхода игроков
        elapsed_ms_ += time_delta_ms;

        std::unordered_map<uint32_t, geom::Point2D> start_positions;
        start_positions.reserve(players_.size());
        for (auto& [id, player] : players_) {
            start_positions[id] = player.GetDog()->GetPosition();
        }

        for (auto& [player_id, player] : players_) {
            auto& dog = player.GetDog();
            const geom::Vec2D speed = dog->GetSpeed();

            // <-- ДОБАВЛЕНО: фиксируем активность, если собака движется
            const bool moving =
                (std::abs(speed.x) > EPSILON || std::abs(speed.y) > EPSILON);
            if (moving) {
                player.SetLastActiveTimeMs(elapsed_ms_);
            }
            if (!moving) continue;

            const Map::Id map_id = GetPlayerMap(player_id);
            const Map* map = FindMap(map_id);
            if (!map) continue;

            const geom::Point2D pos = dog->GetPosition();
            const geom::Point2D target{pos.x + speed.x * dt, pos.y + speed.y * dt};

            if (std::abs(speed.x) > EPSILON) {
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
                geom::Point2D new_pos = target;
                if (speed.x > 0) { if (target.x > max_x) { new_pos.x = max_x; dog->SetSpeed({0.0, 0.0}); } }
                else { if (target.x < min_x) { new_pos.x = min_x; dog->SetSpeed({0.0, 0.0}); } }
                dog->SetPosition(new_pos);
                continue;
            }

            if (std::abs(speed.y) > EPSILON) {
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
                geom::Point2D new_pos = target;
                if (speed.y > 0) { if (target.y > max_y) { new_pos.y = max_y; dog->SetSpeed({0.0, 0.0}); } }
                else { if (target.y < min_y) { new_pos.y = min_y; dog->SetSpeed({0.0, 0.0}); } }
                dog->SetPosition(new_pos);
            }
        }

        HandleGatherAndReturn(start_positions);

        // генерация
        if (loot_gen_ && !players_.empty()) {
            const auto map_id = GetPlayerMap(players_.begin()->first);
            const Map* map = FindMap(map_id);
            if (map) {
                const auto time_delta = std::chrono::milliseconds(time_delta_ms);
                const unsigned looters = static_cast<unsigned>(players_.size());
                const unsigned loot = static_cast<unsigned>(lost_objects_.Size());
                const unsigned to_generate = loot_gen_->Generate(time_delta, loot, looters);

                for (unsigned i = 0; i < to_generate; ++i) {
                    LootType type = 0;
                    if (map->GetLootTypesCount() > 0) {
                        std::uniform_int_distribution<std::uint32_t> type_dist{
                            0, map->GetLootTypesCount() - 1};
                        type = type_dist(gen_);
                    }
                    lost_objects_.Add(type, map->GetLootValue(type),
                                      map->GetRandomPositionOnRoad());
                }
            }
        }

        // elapsed_ms_ += time_delta_ms  уже инкрементирован в начале Tick
        if (tick_listener_) tick_listener_(std::chrono::milliseconds{time_delta_ms});

        // ============================================================
        // <-- ДОБАВЛЕНО: уход на пенсию
        // ============================================================
        if (dog_retirement_ms_ > 0 && !players_.empty()) {
            std::vector<uint32_t> to_retire;
            to_retire.reserve(players_.size());
            for (const auto& [id, player] : players_) {
                if (elapsed_ms_ - player.GetLastActiveTimeMs() >= dog_retirement_ms_) {
                    to_retire.push_back(id);
                }
            }

            for (uint32_t id : to_retire) {
                auto it = players_.find(id);
                if (it == players_.end()) continue;
                const auto& player = it->second;

                if (retirement_listener_) {
                    const double play_time =
                        static_cast<double>(elapsed_ms_ - player.GetJoinTimeMs()) / 1000.0;
                    retirement_listener_->OnPlayerRetired(
                        player.GetName(),
                        static_cast<int>(player.GetDog()->GetScore()),
                        play_time);
                }

                // Удаляем токен
                auto tok_it = player_to_token_.find(id);
                if (tok_it != player_to_token_.end()) {
                    token_to_player_.erase(tok_it->second);
                    player_to_token_.erase(tok_it);
                }

                // Убираем игрока из map_players_
                for (auto& [map_id, ids] : map_players_) {
                    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
                }

                players_.erase(it);
            }
        }
        // ============================================================
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

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

    struct TickEvent {
        enum class Type { Collect, Return };
        double time;
        Type type;
        uint32_t player_id;
        LostObjectId item_id = 0;
    };

    void HandleGatherAndReturn(
        const std::unordered_map<uint32_t, geom::Point2D>& start_positions) {

        if (players_.empty()) return;
        const auto map_id = GetPlayerMap(players_.begin()->first);
        const Map* map_ptr = FindMap(map_id);
        if (!map_ptr) return;
        const Map& map = *map_ptr;

        Provider provider;

        std::vector<LostObjectId> item_ids;
        item_ids.reserve(lost_objects_.Size());
        for (const auto& [id, obj] : lost_objects_.Get()) {
            collision_detector::Item it;
            it.position = obj.pos;
            it.width = 0.0;
            provider.AddItem(it);
            item_ids.push_back(id);
        }

        std::vector<uint32_t> player_ids;
        player_ids.reserve(players_.size());
        for (const auto& [id, player] : players_) {
            const auto s = start_positions.at(id);
            const auto e = player.GetDog()->GetPosition();
            collision_detector::Gatherer g;
            g.start_pos = s;
            g.end_pos = e;
            g.width = 0.3;
            provider.AddGatherer(g);
            player_ids.push_back(id);
        }

        std::vector<TickEvent> events;

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

        for (const auto& office : map.GetOffices()) {
            const geom::Point2D office_pos{
                static_cast<double>(office.GetPosition().x),
                static_cast<double>(office.GetPosition().y)
            };
            for (std::size_t i = 0; i < player_ids.size(); ++i) {
                const auto s = start_positions.at(player_ids[i]);
                const auto e = players_.at(player_ids[i]).GetDog()->GetPosition();
                if (std::abs(e.x - s.x) < 1e-12 && std::abs(e.y - s.y) < 1e-12) continue;
                auto res = collision_detector::TryCollectPoint(s, e, office_pos);
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

        std::sort(events.begin(), events.end(),
                  [](const TickEvent& a, const TickEvent& b) { return a.time < b.time; });

        for (const auto& ev : events) {
            auto pit = players_.find(ev.player_id);
            if (pit == players_.end()) continue;
            auto& dog = pit->second.GetDog();

            if (ev.type == TickEvent::Type::Collect) {
                if (dog->IsBagFull()) continue;
                const LostObject* obj = lost_objects_.Find(ev.item_id);
                if (!obj) continue;
                dog->PutToBag(FoundObject{FoundObject::Id{obj->id}, obj->type});
                lost_objects_.Remove(ev.item_id);
            } else {
                for (const auto& item : dog->GetBagContent()) {
                    dog->AddScore(static_cast<Score>(map.GetLootValue(item.type)));
                }
                dog->EmptyBag();
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

    TickListener tick_listener_;
    int64_t elapsed_ms_ = 0;

    // <-- ДОБАВЛЕНО: retirement
    int64_t dog_retirement_ms_ = 60'000;
    RetirementListener* retirement_listener_ = nullptr;
};

}  // namespace model
