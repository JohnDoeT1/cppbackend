#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <random>
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

using LostObjectId = std::uint32_t;
using LootType = std::uint32_t;
using Score = unsigned;

struct LostObject {
    LostObjectId id = 0;
    LootType type = 0;
    int value = 0;
    geom::Point2D pos;
};

struct FoundObject {
    using Id = util::Tagged<std::uint32_t, FoundObject>;
    Id id{0u};
    LootType type{0u};

    [[nodiscard]] auto operator<=>(const FoundObject&) const = default;
};

// ============================================================
// Road
// ============================================================
class Road {
    struct HorizontalTag { explicit HorizontalTag() = default; };
    struct VerticalTag { explicit VerticalTag() = default; };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept;
    Road(VerticalTag, Point start, Coord end_y) noexcept;

    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;

private:
    Point start_;
    Point end_;
};

// ============================================================
// Building
// ============================================================
class Building {
public:
    explicit Building(Rectangle bounds) noexcept;
    const Rectangle& GetBounds() const noexcept;

private:
    Rectangle bounds_;
};

// ============================================================
// Office
// ============================================================
class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept;

    const Id& GetId() const noexcept;
    Point GetPosition() const noexcept;
    Offset GetOffset() const noexcept;

private:
    Id id_;
    Point position_;
    Offset offset_;
};

// ============================================================
// Dog
// ============================================================
class Dog {
public:
    using Id = util::Tagged<std::uint32_t, Dog>;
    using BagContent = std::vector<FoundObject>;

    Dog() = default;
    Dog(Id id, std::string name, geom::Point2D pos, std::size_t bag_cap);

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const geom::Point2D& GetPosition() const noexcept;
    void SetPosition(geom::Point2D pos) noexcept;

    const geom::Vec2D& GetSpeed() const noexcept;
    void SetSpeed(geom::Vec2D speed) noexcept;

    Direction GetDirection() const noexcept;
    void SetDirection(Direction dir) noexcept;

    std::size_t GetBagCapacity() const noexcept;
    const BagContent& GetBagContent() const noexcept;

    bool IsBagFull() const noexcept;
    bool PutToBag(FoundObject item);
    std::size_t EmptyBag() noexcept;

    Score GetScore() const noexcept;
    void AddScore(Score score) noexcept;

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
// RetirementListener
// ============================================================
class RetirementListener {
public:
    virtual void OnPlayerRetired(const std::string& name,
                                 int score,
                                 double play_time_sec) = 0;
    virtual ~RetirementListener() = default;
};

// ============================================================
// Player
// ============================================================
class Player {
public:
    using Id = std::uint32_t;

    Player() = default;
    Player(Id id, std::string name, geom::Point2D pos, std::size_t bag_cap);
    Player(Id id, DogPtr dog);

    Id GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const DogPtr& GetDog() const noexcept;

    bool IsBagFull() const noexcept;
    const std::vector<FoundObject>& GetBag() const noexcept;
    std::size_t GetBagCapacity() const noexcept;
    int GetScore() const noexcept;

    int64_t GetJoinTimeMs() const noexcept;
    void SetJoinTimeMs(int64_t value) noexcept;

    int64_t GetLastActiveTimeMs() const noexcept;
    void SetLastActiveTimeMs(int64_t value) noexcept;

private:
    Id id_ = 0;
    DogPtr dog_;
    int64_t join_time_ms_ = 0;
    int64_t last_active_time_ms_ = 0;
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
        std::uint32_t loot_types_count, std::size_t bag_capacity) noexcept;
    Map(Id id, std::string name, double dog_speed,
        std::uint32_t loot_types_count) noexcept;
    Map(Id id, std::string name, double dog_speed = 1.0) noexcept;

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;
    double GetDogSpeed() const noexcept;
    std::uint32_t GetLootTypesCount() const noexcept;
    std::size_t GetBagCapacity() const noexcept;

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);

    geom::Point2D GetStartPositionOnRoad() const;
    geom::Point2D GetRandomPositionOnRoad() const;
    bool IsPositionOnRoad(const geom::Point2D& pos) const;

    void SetLootValues(std::vector<int> values);
    int GetLootValue(LootType type) const;

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

    LostObjectId Add(LootType type, int value, geom::Point2D pos);
    LostObjectId Add(LootType type, geom::Point2D pos);
    bool Remove(LostObjectId id);
    const LostObject* Find(LostObjectId id) const;

    const Storage& Get() const noexcept;
    std::size_t Size() const noexcept;

    LostObjectId GetNextId() const noexcept;
    void SetNextId(LostObjectId id) noexcept;
    void SetStorage(Storage storage, LostObjectId next_id);

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

    Game(std::shared_ptr<loot_gen::LootGenerator> loot_gen,
         double default_dog_speed = 1.0);
    explicit Game(double default_dog_speed = 1.0);

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept;
    const Map* FindMap(const Map::Id& id) const noexcept;

    uint32_t AddPlayer(const std::string& name, const Map::Id& map_id);

    const Players& GetPlayers() const noexcept;
    Players& GetPlayers() noexcept;
    void SetPlayers(Players players);

    const std::vector<uint32_t>& GetPlayersOnMap(const Map::Id& map_id) const;

    bool HasPlayer(uint32_t player_id) const noexcept;

    void SetPlayerToken(const std::string& token, uint32_t player_id);
    std::optional<uint32_t> GetPlayerIdByToken(const std::string& token) const;

    Map::Id GetPlayerMap(uint32_t player_id) const;

    void SetRandomizeSpawnPoints(bool value);
    const Map* GetMapById(const Map::Id& map_id) const;

    const LostObjects& GetLostObjects() const noexcept;
    LostObjects& GetLostObjects() noexcept;

    // Слушатель и время
    void SetTickListener(TickListener listener);
    int64_t GetElapsedMs() const noexcept;
    void SetElapsedMs(int64_t value) noexcept;

    // Retirement
    void SetDogRetirementTime(int64_t ms) noexcept;
    int64_t GetDogRetirementTime() const noexcept;
    void SetRetirementListener(RetirementListener* listener) noexcept;

    // Аксессоры для сериализации
    TokenToPlayerId& GetTokenToPlayer() noexcept;
    const TokenToPlayerId& GetTokenToPlayer() const noexcept;

    MapPlayers& GetMapPlayers() noexcept;
    const MapPlayers& GetMapPlayers() const noexcept;

    std::unordered_map<uint32_t, std::string>& GetPlayerToToken() noexcept;
    const std::unordered_map<uint32_t, std::string>& GetPlayerToToken() const noexcept;

    void SetTokenToPlayer(TokenToPlayerId tokens);
    void SetPlayerToToken(std::unordered_map<uint32_t, std::string> player_to_token);
    void SetMapPlayers(MapPlayers map_players);

    uint32_t GetNextPlayerId() const noexcept;
    void SetNextPlayerId(uint32_t value) noexcept;

    void SetPlayerDirection(uint32_t player_id, const std::string& direction);
    void Tick(int time_delta_ms);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    class Provider : public collision_detector::ItemGathererProvider {
    public:
        void AddItem(collision_detector::Item item);
        void AddGatherer(collision_detector::Gatherer gatherer);

        size_t ItemsCount() const override;
        collision_detector::Item GetItem(size_t index) const override;
        size_t GatherersCount() const override;
        collision_detector::Gatherer GetGatherer(size_t index) const override;

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
        const std::unordered_map<uint32_t, geom::Point2D>& start_positions);

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

    int64_t dog_retirement_ms_ = 60'000;
    RetirementListener* retirement_listener_ = nullptr;
};

}  // namespace model
