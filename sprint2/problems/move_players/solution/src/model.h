#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <algorithm>
#include <random>

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

    Map(Id id, std::string name, double dog_speed = 1.0) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_speed_(dog_speed) {
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

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);
    
    // Получить случайную позицию на дороге
    Position GetRandomPositionOnRoad() const {
        if (roads_.empty()) {
            return Position{0.0, 0.0};
        }
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // Выбираем случайную дорогу
        std::uniform_int_distribution<> road_dist(0, roads_.size() - 1);
        const Road& road = roads_[road_dist(gen)];
        
        // Выбираем случайную точку на дороге
        if (road.IsHorizontal()) {
            std::uniform_real_distribution<> dist(road.GetStart().x, road.GetEnd().x);
            return Position{dist(gen), static_cast<double>(road.GetStart().y)};
        } else {
            std::uniform_real_distribution<> dist(road.GetStart().y, road.GetEnd().y);
            return Position{static_cast<double>(road.GetStart().x), dist(gen)};
        }
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    double dog_speed_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Game {
public:
    using Maps = std::vector<Map>;
    using Players = std::unordered_map<uint32_t, Player>;
    using TokenToPlayerId = std::unordered_map<std::string, uint32_t>;
    using MapPlayers = std::unordered_map<Map::Id, std::vector<uint32_t>, util::TaggedHasher<Map::Id>>;

    Game(double default_dog_speed = 1.0) 
        : default_dog_speed_(default_dog_speed) {
    }

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }
    
    uint32_t AddPlayer(const std::string& name, const Map::Id& map_id) {
        uint32_t player_id = next_player_id_++;
        
        // Находим карту и получаем случайную позицию на дороге
        const Map* map = FindMap(map_id);
        Position pos;
        if (map) {
            pos = map->GetRandomPositionOnRoad();
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
    
    const Map* GetMapById(const Map::Id& map_id) const {
        return FindMap(map_id);
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
            // Направление не меняется при остановке
        }
        
        dog->SetSpeed(new_speed);
        if (direction != "") {
            dog->SetDirection(new_direction);
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
};

}  // namespace model
