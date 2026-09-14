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

// Направление движения
enum class Direction {
    NORTH = 0,  // U
    SOUTH = 1,  // D
    WEST = 2,   // L
    EAST = 3    // R
};

// Скорость
struct Speed {
    double vx = 0.0;
    double vy = 0.0;
};

// Позиция (вещественные координаты)
struct Position {
    double x = 0.0;
    double y = 0.0;
};

// Потерянный объект
using LostObjectId = std::uint32_t;
using LootType = std::uint32_t;

struct LostObject {
    LootType type = 0;
    Position pos;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Dog {
public:
    Dog(std::string name, Position pos)
        : name_(std::move(name))
        , position_(pos) {
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Position& GetPosition() const noexcept {
        return position_;
    }

    void SetPosition(const Position& pos) noexcept {
        position_ = pos;
    }

    const Speed& GetSpeed() const noexcept {
        return speed_;
    }

    void SetSpeed(const Speed& speed) noexcept {
        speed_ = speed;
    }

    Direction GetDirection() const noexcept {
        return direction_;
    }

    void SetDirection(Direction dir) noexcept {
        direction_ = dir;
    }

private:
    std::string name_;
    Position position_;
    Speed speed_;
    Direction direction_ = Direction::NORTH;
};

class Player {
public:
    using Id = uint32_t;

    Player(Id id, std::string name, Position pos)
        : id_(id)
        , dog_(std::make_shared<Dog>(std::move(name), pos)) {
    }

    Id GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return dog_->GetName();
    }

    const std::shared_ptr<Dog>& GetDog() const noexcept {
        return dog_;
    }

private:
    Id id_;
    std::shared_ptr<Dog> dog_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name, double dog_speed, std::uint32_t loot_types_count) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_speed_(dog_speed)
        , loot_types_count_(loot_types_count) {
    }

    // Старый конструктор — для обратной совместимости (0 типов трофеев).
    Map(Id id, std::string name, double dog_speed = 1.0) noexcept
        : Map(std::move(id), std::move(name), dog_speed, 0) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    double GetDogSpeed() const noexcept {
        return dog_speed_;
    }

    std::uint32_t GetLootTypesCount() const noexcept {
        return loot_types_count_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    // Получить начальную позицию на первой дороге
    Position GetStartPositionOnRoad() const {
        if (roads_.empty()) {
            return Position{0.0, 0.0};
        }

        const Road& road = roads_[0];
        return Position{static_cast<double>(road.GetStart().x),
                        static_cast<double>(road.GetStart().y)};
    }

    // Получить случайную позицию на дороге
    Position GetRandomPositionOnRoad() const {
        if (roads_.empty()) {
            return Position{0.0, 0.0};
        }

        static std::random_device rd;
        static std::mt19937 gen(rd());

        std::uniform_int_distribution<> road_dist(0, static_cast<int>(roads_.size()) - 1);
        const Road& road = roads_[road_dist(gen)];

        if (road.IsHorizontal()) {
            std::uniform_real_distribution<> dist(road.GetStart().x, road.GetEnd().x);
            return Position{dist(gen), static_cast<double>(road.GetStart().y)};
        }
        std::uniform_real_distribution<> dist(road.GetStart().y, road.GetEnd().y);
        return Position{static_cast<double>(road.GetStart().x), dist(gen)};
    }

    // Проверить, находится ли позиция в пределах дороги
    bool IsPositionOnRoad(const Position& pos) const {
        constexpr double HALF_WIDTH = 0.4;

        for (const auto& road : roads_) {
            const Point start = road.GetStart();
            const Point end = road.GetEnd();

            if (road.IsHorizontal()) {
                const double min_x =
                    std::min(static_cast<double>(start.x), static_cast<double>(end.x));
                const double max_x =
                    std::max(static_cast<double>(start.x), static_cast<double>(end.x));
                const double road_y = static_cast<double>(start.y);

                if (std::abs(pos.y - road_y) <= HALF_WIDTH &&
                    pos.x >= min_x - HALF_WIDTH &&
                    pos.x <= max_x + HALF_WIDTH) {
                    return true;
                }
            } else {
                const double min_y =
                    std::min(static_cast<double>(start.y), static_cast<double>(end.y));
                const double max_y =
                    std::max(static_cast<double>(start.y), static_cast<double>(end.y));
                const double road_x = static_cast<double>(start.x);

                if (std::abs(pos.x - road_x) <= HALF_WIDTH &&
                    pos.y >= min_y - HALF_WIDTH &&
                    pos.y <= max_y + HALF_WIDTH) {
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
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

// Хранилище потерянных объектов
class LostObjects {
public:
    using Storage = std::unordered_map<LostObjectId, LostObject>;

    void Add(LostObject obj) {
        objects_.emplace(next_id_++, std::move(obj));
    }

    const Storage& Get() const noexcept {
        return objects_;
    }

    std::size_t Size() const noexcept {
        return objects_.size();
    }

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

    // Конструктор с генератором (для реального сервера)
    Game(std::shared_ptr<loot_gen::LootGenerator> loot_gen, double default_dog_speed = 1.0)
        : loot_gen_{std::move(loot_gen)}
        , default_dog_speed_(default_dog_speed) {
    }

    // Конструктор без генератора (для тестов / обратной совместимости)
    Game(double default_dog_speed = 1.0)
        : Game(nullptr, default_dog_speed) {
    }

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        auto it = map_id_to_index_.find(id);
        return it != map_id_to_index_.end() ? &maps_.at(it->second) : nullptr;
    }

    uint32_t AddPlayer(const std::string& name, const Map::Id& map_id) {
        uint32_t player_id = next_player_id_++;

        const Map* map = FindMap(map_id);
        Position pos;
        if (map) {
            if (randomize_spawn_points_) {
                pos = map->GetRandomPositionOnRoad();
            } else {
                pos = map->GetStartPositionOnRoad();
            }
        }

        Player player(player_id, name, pos);
        players_.emplace(player_id, player);
        map_players_[map_id].push_back(player_id);
        return player_id;
    }

    const Players& GetPlayers() const noexcept {
        return players_;
    }

    const std::vector<uint32_t>& GetPlayersOnMap(const Map::Id& map_id) const {
        static const std::vector<uint32_t> empty;
        auto it = map_players_.find(map_id);
        if (it != map_players_.end()) {
            return it->second;
        }
        return empty;
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
        if (it != token_to_player_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    model::Map::Id GetPlayerMap(uint32_t player_id) const {
        for (const auto& [map_id, players] : map_players_) {
            if (std::find(players.begin(), players.end(), player_id) != players.end()) {
                return map_id;
            }
        }
        throw std::runtime_error("Player not found on any map");
    }

    void SetRandomizeSpawnPoints(bool value) {
        randomize_spawn_points_ = value;
    }

    const Map* GetMapById(const Map::Id& map_id) const {
        return FindMap(map_id);
    }

    const LostObjects& GetLostObjects() const noexcept {
        return lost_objects_;
    }

    // Установить направление движения игрока
    void SetPlayerDirection(uint32_t player_id, const std::string& direction) {
        auto it = players_.find(player_id);
        if (it == players_.end()) {
            return;
        }

        auto& dog = it->second.GetDog();
        model::Map::Id map_id = GetPlayerMap(player_id);
        const Map* map = FindMap(map_id);
        if (!map) {
            return;
        }

        double speed = map->GetDogSpeed();
        Speed new_speed{0.0, 0.0};
        Direction new_direction = dog->GetDirection();

        if (direction == "L") {
            new_speed = Speed{-speed, 0.0};
            new_direction = Direction::WEST;
        } else if (direction == "R") {
            new_speed = Speed{speed, 0.0};
            new_direction = Direction::EAST;
        } else if (direction == "U") {
            new_speed = Speed{0.0, -speed};
            new_direction = Direction::NORTH;
        } else if (direction == "D") {
            new_speed = Speed{0.0, speed};
            new_direction = Direction::SOUTH;
        } else if (direction == "") {
            new_speed = Speed{0.0, 0.0};
        }

        dog->SetSpeed(new_speed);
        if (direction != "") {
            dog->SetDirection(new_direction);
        }
    }

    // Обновить состояние игры на время time_delta (в миллисекундах)
    void Tick(int time_delta_ms) {
        // --- 1. Движение собак ---
        constexpr double HALF_WIDTH = 0.4;
        constexpr double EPSILON = 1e-9;
        const double dt = time_delta_ms / 1000.0;

        for (auto& [player_id, player] : players_) {
            auto& dog = player.GetDog();
            const Speed speed = dog->GetSpeed();

            if (std::abs(speed.vx) < EPSILON &&
                std::abs(speed.vy) < EPSILON) {
                continue;
            }

            const Map::Id map_id = GetPlayerMap(player_id);
            const Map* map = FindMap(map_id);
            if (!map) {
                continue;
            }

            const Position pos = dog->GetPosition();
            const Position target{pos.x + speed.vx * dt, pos.y + speed.vy * dt};

            if (std::abs(speed.vx) > EPSILON) {
                double min_x = pos.x;
                double max_x = pos.x;
                bool changed = true;
                while (changed) {
                    changed = false;
                    for (const auto& road : map->GetRoads()) {
                        double road_min;
                        double road_max;
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
                if (speed.vx > 0) {
                    if (target.x > max_x) { new_pos.x = max_x; dog->SetSpeed(Speed{0.0, 0.0}); }
                } else {
                    if (target.x < min_x) { new_pos.x = min_x; dog->SetSpeed(Speed{0.0, 0.0}); }
                }
                dog->SetPosition(new_pos);
                continue;
            }

            if (std::abs(speed.vy) > EPSILON) {
                double min_y = pos.y;
                double max_y = pos.y;
                bool changed = true;
                while (changed) {
                    changed = false;
                    for (const auto& road : map->GetRoads()) {
                        double road_min;
                        double road_max;
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
                if (speed.vy > 0) {
                    if (target.y > max_y) { new_pos.y = max_y; dog->SetSpeed(Speed{0.0, 0.0}); }
                } else {
                    if (target.y < min_y) { new_pos.y = min_y; dog->SetSpeed(Speed{0.0, 0.0}); }
                }
                dog->SetPosition(new_pos);
            }
        }

        // --- 2. Генерация потерянных объектов ---
        if (!loot_gen_) {
            return;
        }

        const auto time_delta = std::chrono::milliseconds(time_delta_ms);
        const unsigned looters = static_cast<unsigned>(players_.size());
        const unsigned loot = static_cast<unsigned>(lost_objects_.Size());
        const unsigned to_generate = loot_gen_->Generate(time_delta, loot, looters);

        if (to_generate == 0 || maps_.empty()) {
            return;
        }

        const Map& map = maps_.front();
        for (unsigned i = 0; i < to_generate; ++i) {
            LostObject obj;
            if (map.GetLootTypesCount() > 0) {
                std::uniform_int_distribution<std::uint32_t> type_dist{
                    0, map.GetLootTypesCount() - 1};
                obj.type = type_dist(gen_);
            }
            obj.pos = map.GetRandomPositionOnRoad();
            lost_objects_.Add(std::move(obj));
        }
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

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
