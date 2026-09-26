#include "model.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace model {

using namespace std::literals;

// ============================================================
// Road
// ============================================================
Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
    : start_{start}
    , end_{end_x, start.y} {
}

Road::Road(VerticalTag, Point start, Coord end_y) noexcept
    : start_{start}
    , end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

// ============================================================
// Building
// ============================================================
Building::Building(Rectangle bounds) noexcept
    : bounds_{bounds} {
}

const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

// ============================================================
// Office
// ============================================================
Office::Office(Id id, Point position, Offset offset) noexcept
    : id_{std::move(id)}
    , position_{position}
    , offset_{offset} {
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

Point Office::GetPosition() const noexcept {
    return position_;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}

// ============================================================
// Dog
// ============================================================
Dog::Dog(Id id, std::string name, geom::Point2D pos, std::size_t bag_cap)
    : id_(std::move(id))
    , name_(std::move(name))
    , position_(pos)
    , bag_cap_(bag_cap) {
    bag_.reserve(bag_cap);
}

const Dog::Id& Dog::GetId() const noexcept {
    return id_;
}

const std::string& Dog::GetName() const noexcept {
    return name_;
}

const geom::Point2D& Dog::GetPosition() const noexcept {
    return position_;
}

void Dog::SetPosition(geom::Point2D pos) noexcept {
    position_ = pos;
}

const geom::Vec2D& Dog::GetSpeed() const noexcept {
    return speed_;
}

void Dog::SetSpeed(geom::Vec2D speed) noexcept {
    speed_ = speed;
}

Direction Dog::GetDirection() const noexcept {
    return direction_;
}

void Dog::SetDirection(Direction dir) noexcept {
    direction_ = dir;
}

std::size_t Dog::GetBagCapacity() const noexcept {
    return bag_cap_;
}

const Dog::BagContent& Dog::GetBagContent() const noexcept {
    return bag_;
}

bool Dog::IsBagFull() const noexcept {
    return bag_.size() >= bag_cap_;
}

bool Dog::PutToBag(FoundObject item) {
    if (IsBagFull()) {
        return false;
    }
    bag_.push_back(item);
    return true;
}

std::size_t Dog::EmptyBag() noexcept {
    const std::size_t removed = bag_.size();
    bag_.clear();
    return removed;
}

Score Dog::GetScore() const noexcept {
    return score_;
}

void Dog::AddScore(Score score) noexcept {
    score_ += score;
}

// ============================================================
// Player
// ============================================================
Player::Player(Id id, std::string name, geom::Point2D pos, std::size_t bag_cap)
    : id_(id)
    , dog_(std::make_shared<Dog>(Dog::Id{id}, std::move(name), pos, bag_cap)) {
}

Player::Player(Id id, DogPtr dog)
    : id_(id)
    , dog_(std::move(dog)) {
}

Player::Id Player::GetId() const noexcept {
    return id_;
}

const std::string& Player::GetName() const noexcept {
    return dog_->GetName();
}

const DogPtr& Player::GetDog() const noexcept {
    return dog_;
}

bool Player::IsBagFull() const noexcept {
    return dog_->IsBagFull();
}

const std::vector<FoundObject>& Player::GetBag() const noexcept {
    return dog_->GetBagContent();
}

std::size_t Player::GetBagCapacity() const noexcept {
    return dog_->GetBagCapacity();
}

int Player::GetScore() const noexcept {
    return static_cast<int>(dog_->GetScore());
}

int64_t Player::GetJoinTimeMs() const noexcept {
    return join_time_ms_;
}

void Player::SetJoinTimeMs(int64_t value) noexcept {
    join_time_ms_ = value;
}

int64_t Player::GetLastActiveTimeMs() const noexcept {
    return last_active_time_ms_;
}

void Player::SetLastActiveTimeMs(int64_t value) noexcept {
    last_active_time_ms_ = value;
}

// ============================================================
// Map
// ============================================================
Map::Map(Id id, std::string name, double dog_speed,
         std::uint32_t loot_types_count, std::size_t bag_capacity) noexcept
    : id_(std::move(id))
    , name_(std::move(name))
    , dog_speed_(dog_speed)
    , loot_types_count_(loot_types_count)
    , bag_capacity_(bag_capacity) {
}

Map::Map(Id id, std::string name, double dog_speed,
         std::uint32_t loot_types_count) noexcept
    : Map(std::move(id), std::move(name), dog_speed, loot_types_count, 3) {
}

Map::Map(Id id, std::string name, double dog_speed) noexcept
    : Map(std::move(id), std::move(name), dog_speed, 0, 3) {
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

double Map::GetDogSpeed() const noexcept {
    return dog_speed_;
}

std::uint32_t Map::GetLootTypesCount() const noexcept {
    return loot_types_count_;
}

std::size_t Map::GetBagCapacity() const noexcept {
    return bag_capacity_;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }
    const size_t index = offices_.size();
    Office& added_office = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(added_office.GetId(), index);
    } catch (const std::exception&) {
        offices_.pop_back();
        throw;
    }
}

geom::Point2D Map::GetStartPositionOnRoad() const {
    if (roads_.empty()) {
        return geom::Point2D{0.0, 0.0};
    }
    const Road& road = roads_[0];
    return geom::Point2D{static_cast<double>(road.GetStart().x),
                         static_cast<double>(road.GetStart().y)};
}

geom::Point2D Map::GetRandomPositionOnRoad() const {
    if (roads_.empty()) {
        return geom::Point2D{0.0, 0.0};
    }

    static std::random_device random_device;
    static std::mt19937 generator(random_device());

    std::uniform_int_distribution<> road_dist(0, static_cast<int>(roads_.size()) - 1);
    const Road& road = roads_[road_dist(generator)];

    const Point start = road.GetStart();
    const Point end = road.GetEnd();

    if (road.IsHorizontal()) {
        const double min_x = std::min(static_cast<double>(start.x),
                                      static_cast<double>(end.x));
        const double max_x = std::max(static_cast<double>(start.x),
                                      static_cast<double>(end.x));
        std::uniform_real_distribution<> dist(min_x, max_x);
        return geom::Point2D{dist(generator), static_cast<double>(start.y)};
    }

    const double min_y = std::min(static_cast<double>(start.y),
                                  static_cast<double>(end.y));
    const double max_y = std::max(static_cast<double>(start.y),
                                  static_cast<double>(end.y));
    std::uniform_real_distribution<> dist(min_y, max_y);
    return geom::Point2D{static_cast<double>(start.x), dist(generator)};
}

bool Map::IsPositionOnRoad(const geom::Point2D& pos) const {
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

void Map::SetLootValues(std::vector<int> values) {
    loot_values_ = std::move(values);
}

int Map::GetLootValue(LootType type) const {
    if (type < loot_values_.size()) {
        return loot_values_[type];
    }
    return 0;
}

// ============================================================
// LostObjects
// ============================================================
LostObjectId LostObjects::Add(LootType type, int value, geom::Point2D pos) {
    const LostObjectId id = next_id_++;
    LostObject lost_object{id, type, value, pos};
    objects_.emplace(id, std::move(lost_object));
    return id;
}

LostObjectId LostObjects::Add(LootType type, geom::Point2D pos) {
    return Add(type, 0, pos);
}

bool LostObjects::Remove(LostObjectId id) {
    return objects_.erase(id) > 0;
}

const LostObject* LostObjects::Find(LostObjectId id) const {
    const auto it = objects_.find(id);
    return it == objects_.end() ? nullptr : &it->second;
}

const LostObjects::Storage& LostObjects::Get() const noexcept {
    return objects_;
}

std::size_t LostObjects::Size() const noexcept {
    return objects_.size();
}

LostObjectId LostObjects::GetNextId() const noexcept {
    return next_id_;
}

void LostObjects::SetNextId(LostObjectId id) noexcept {
    next_id_ = id;
}

void LostObjects::SetStorage(Storage storage, LostObjectId next_id) {
    objects_ = std::move(storage);
    next_id_ = next_id;
}

// ============================================================
// Game
// ============================================================
Game::Game(std::shared_ptr<loot_gen::LootGenerator> loot_gen, double default_dog_speed)
    : loot_gen_{std::move(loot_gen)}
    , default_dog_speed_(default_dog_speed) {
}

Game::Game(double default_dog_speed)
    : Game(nullptr, default_dog_speed) {
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (const std::exception&) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    const auto it = map_id_to_index_.find(id);
    return it != map_id_to_index_.end() ? &maps_.at(it->second) : nullptr;
}

uint32_t Game::AddPlayer(const std::string& name, const Map::Id& map_id) {
    const uint32_t player_id = next_player_id_++;
    const Map* map = FindMap(map_id);
    geom::Point2D pos{0.0, 0.0};
    std::size_t cap = 3;
    if (map) {
        pos = randomize_spawn_points_ ? map->GetRandomPositionOnRoad()
                                      : map->GetStartPositionOnRoad();
        cap = map->GetBagCapacity();
    }
    Player player(player_id, name, pos, cap);
    player.SetJoinTimeMs(elapsed_ms_);
    player.SetLastActiveTimeMs(elapsed_ms_);
    players_.emplace(player_id, std::move(player));
    map_players_[map_id].push_back(player_id);
    return player_id;
}

const Game::Players& Game::GetPlayers() const noexcept {
    return players_;
}

Game::Players& Game::GetPlayers() noexcept {
    return players_;
}

void Game::SetPlayers(Players players) {
    players_ = std::move(players);
}

const std::vector<uint32_t>& Game::GetPlayersOnMap(const Map::Id& map_id) const {
    static const std::vector<uint32_t> empty;
    const auto it = map_players_.find(map_id);
    return it == map_players_.end() ? empty : it->second;
}

bool Game::HasPlayer(uint32_t player_id) const noexcept {
    return players_.contains(player_id);
}

void Game::SetPlayerToken(const std::string& token, uint32_t player_id) {
    token_to_player_[token] = player_id;
    player_to_token_[player_id] = token;
}

std::optional<uint32_t> Game::GetPlayerIdByToken(const std::string& token) const {
    const auto it = token_to_player_.find(token);
    return it == token_to_player_.end() ? std::nullopt : std::optional{it->second};
}

Map::Id Game::GetPlayerMap(uint32_t player_id) const {
    for (const auto& [map_id, players] : map_players_) {
        if (std::find(players.begin(), players.end(), player_id) != players.end()) {
            return map_id;
        }
    }
    throw std::runtime_error("Player not found on any map");
}

void Game::SetRandomizeSpawnPoints(bool value) {
    randomize_spawn_points_ = value;
}

const Map* Game::GetMapById(const Map::Id& map_id) const {
    return FindMap(map_id);
}

const LostObjects& Game::GetLostObjects() const noexcept {
    return lost_objects_;
}

LostObjects& Game::GetLostObjects() noexcept {
    return lost_objects_;
}

void Game::SetTickListener(TickListener listener) {
    tick_listener_ = std::move(listener);
}

int64_t Game::GetElapsedMs() const noexcept {
    return elapsed_ms_;
}

void Game::SetElapsedMs(int64_t value) noexcept {
    elapsed_ms_ = value;
}

void Game::SetDogRetirementTime(int64_t ms) noexcept {
    dog_retirement_ms_ = ms;
}

int64_t Game::GetDogRetirementTime() const noexcept {
    return dog_retirement_ms_;
}

void Game::SetRetirementListener(RetirementListener* listener) noexcept {
    retirement_listener_ = listener;
}

Game::TokenToPlayerId& Game::GetTokenToPlayer() noexcept {
    return token_to_player_;
}

const Game::TokenToPlayerId& Game::GetTokenToPlayer() const noexcept {
    return token_to_player_;
}

Game::MapPlayers& Game::GetMapPlayers() noexcept {
    return map_players_;
}

const Game::MapPlayers& Game::GetMapPlayers() const noexcept {
    return map_players_;
}

std::unordered_map<uint32_t, std::string>& Game::GetPlayerToToken() noexcept {
    return player_to_token_;
}

const std::unordered_map<uint32_t, std::string>& Game::GetPlayerToToken() const noexcept {
    return player_to_token_;
}

void Game::SetTokenToPlayer(TokenToPlayerId tokens) {
    token_to_player_ = std::move(tokens);
}

void Game::SetPlayerToToken(std::unordered_map<uint32_t, std::string> player_to_token) {
    player_to_token_ = std::move(player_to_token);
}

void Game::SetMapPlayers(MapPlayers map_players) {
    map_players_ = std::move(map_players);
}

uint32_t Game::GetNextPlayerId() const noexcept {
    return next_player_id_;
}

void Game::SetNextPlayerId(uint32_t value) noexcept {
    next_player_id_ = value;
}

void Game::SetPlayerDirection(uint32_t player_id, const std::string& direction) {
    auto it = players_.find(player_id);
    if (it == players_.end()) {
        return;
    }
    auto& dog = it->second.GetDog();
    const Map::Id map_id = GetPlayerMap(player_id);
    const Map* map = FindMap(map_id);
    if (!map) {
        return;
    }

    const double speed = map->GetDogSpeed();
    geom::Vec2D new_speed{0.0, 0.0};
    Direction new_direction = dog->GetDirection();

    if (direction == "L") {
        new_speed = geom::Vec2D{-speed, 0.0};
        new_direction = Direction::WEST;
    } else if (direction == "R") {
        new_speed = geom::Vec2D{speed, 0.0};
        new_direction = Direction::EAST;
    } else if (direction == "U") {
        new_speed = geom::Vec2D{0.0, -speed};
        new_direction = Direction::NORTH;
    } else if (direction == "D") {
        new_speed = geom::Vec2D{0.0, speed};
        new_direction = Direction::SOUTH;
    } else if (direction == "") {
        new_speed = geom::Vec2D{0.0, 0.0};
    }

    dog->SetSpeed(new_speed);
    if (direction != "") {
        dog->SetDirection(new_direction);
        it->second.SetLastActiveTimeMs(elapsed_ms_);
    }
}

void Game::Tick(int time_delta_ms) {
    constexpr double HALF_WIDTH = 0.4;
    constexpr double EPSILON = 1e-9;
    const double dt = time_delta_ms / 1000.0;

    // Инкрементируем elapsed_ms_ до обхода игроков
    elapsed_ms_ += time_delta_ms;

    std::unordered_map<uint32_t, geom::Point2D> start_positions;
    start_positions.reserve(players_.size());
    for (auto& [id, player] : players_) {
        start_positions[id] = player.GetDog()->GetPosition();
    }

    for (auto& [player_id, player] : players_) {
        auto& dog = player.GetDog();
        const geom::Vec2D speed = dog->GetSpeed();

        const bool moving =
            (std::abs(speed.x) > EPSILON || std::abs(speed.y) > EPSILON);
        if (moving) {
            player.SetLastActiveTimeMs(elapsed_ms_);
        }
        if (!moving) {
            continue;
        }

        const Map::Id map_id = GetPlayerMap(player_id);
        const Map* map = FindMap(map_id);
        if (!map) {
            continue;
        }

        const geom::Point2D pos = dog->GetPosition();
        const geom::Point2D target{pos.x + speed.x * dt, pos.y + speed.y * dt};

        if (std::abs(speed.x) > EPSILON) {
            double min_x = pos.x;
            double max_x = pos.x;
            bool changed = true;
            while (changed) {
                changed = false;
                for (const auto& road : map->GetRoads()) {
                    double road_min = 0.0;
                    double road_max = 0.0;
                    if (road.IsHorizontal()) {
                        const double road_y = static_cast<double>(road.GetStart().y);
                        if (std::abs(pos.y - road_y) > HALF_WIDTH) {
                            continue;
                        }
                        road_min = std::min(static_cast<double>(road.GetStart().x),
                                            static_cast<double>(road.GetEnd().x)) - HALF_WIDTH;
                        road_max = std::max(static_cast<double>(road.GetStart().x),
                                            static_cast<double>(road.GetEnd().x)) + HALF_WIDTH;
                    } else {
                        const double road_x = static_cast<double>(road.GetStart().x);
                        const double road_min_y =
                            std::min(static_cast<double>(road.GetStart().y),
                                     static_cast<double>(road.GetEnd().y)) - HALF_WIDTH;
                        const double road_max_y =
                            std::max(static_cast<double>(road.GetStart().y),
                                     static_cast<double>(road.GetEnd().y)) + HALF_WIDTH;
                        if (pos.y < road_min_y || pos.y > road_max_y) {
                            continue;
                        }
                        road_min = road_x - HALF_WIDTH;
                        road_max = road_x + HALF_WIDTH;
                    }
                    if (road_max >= min_x && road_min <= max_x) {
                        if (road_min < min_x) {
                            min_x = road_min;
                            changed = true;
                        }
                        if (road_max > max_x) {
                            max_x = road_max;
                            changed = true;
                        }
                    }
                }
            }
            geom::Point2D new_position = target;
            if (speed.x > 0) {
                if (target.x > max_x) {
                    new_position.x = max_x;
                    dog->SetSpeed({0.0, 0.0});
                }
            } else {
                if (target.x < min_x) {
                    new_position.x = min_x;
                    dog->SetSpeed({0.0, 0.0});
                }
            }
            dog->SetPosition(new_position);
            continue;
        }

        if (std::abs(speed.y) > EPSILON) {
            double min_y = pos.y;
            double max_y = pos.y;
            bool changed = true;
            while (changed) {
                changed = false;
                for (const auto& road : map->GetRoads()) {
                    double road_min = 0.0;
                    double road_max = 0.0;
                    if (road.IsVertical()) {
                        const double road_x = static_cast<double>(road.GetStart().x);
                        if (std::abs(pos.x - road_x) > HALF_WIDTH) {
                            continue;
                        }
                        road_min = std::min(static_cast<double>(road.GetStart().y),
                                            static_cast<double>(road.GetEnd().y)) - HALF_WIDTH;
                        road_max = std::max(static_cast<double>(road.GetStart().y),
                                            static_cast<double>(road.GetEnd().y)) + HALF_WIDTH;
                    } else {
                        const double road_y = static_cast<double>(road.GetStart().y);
                        const double road_min_x =
                            std::min(static_cast<double>(road.GetStart().x),
                                     static_cast<double>(road.GetEnd().x)) - HALF_WIDTH;
                        const double road_max_x =
                            std::max(static_cast<double>(road.GetStart().x),
                                     static_cast<double>(road.GetEnd().x)) + HALF_WIDTH;
                        if (pos.x < road_min_x || pos.x > road_max_x) {
                            continue;
                        }
                        road_min = road_y - HALF_WIDTH;
                        road_max = road_y + HALF_WIDTH;
                    }
                    if (road_max >= min_y && road_min <= max_y) {
                        if (road_min < min_y) {
                            min_y = road_min;
                            changed = true;
                        }
                        if (road_max > max_y) {
                            max_y = road_max;
                            changed = true;
                        }
                    }
                }
            }
            geom::Point2D new_position = target;
            if (speed.y > 0) {
                if (target.y > max_y) {
                    new_position.y = max_y;
                    dog->SetSpeed({0.0, 0.0});
                }
            } else {
                if (target.y < min_y) {
                    new_position.y = min_y;
                    dog->SetSpeed({0.0, 0.0});
                }
            }
            dog->SetPosition(new_position);
        }
    }

    HandleGatherAndReturn(start_positions);

    // Генерация новых потерянных предметов
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

    if (tick_listener_) {
        tick_listener_(std::chrono::milliseconds{time_delta_ms});
    }

    // Уход на пенсию
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
            if (it == players_.end()) {
                continue;
            }
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
            auto token_it = player_to_token_.find(id);
            if (token_it != player_to_token_.end()) {
                token_to_player_.erase(token_it->second);
                player_to_token_.erase(token_it);
            }

            // Убираем игрока из map_players_
            for (auto& [map_id, ids] : map_players_) {
                ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
            }

            players_.erase(it);
        }
    }
}

// ============================================================
// Provider
// ============================================================
void Game::Provider::AddItem(collision_detector::Item item) {
    items_.push_back(item);
}

void Game::Provider::AddGatherer(collision_detector::Gatherer gatherer) {
    gatherers_.push_back(gatherer);
}

size_t Game::Provider::ItemsCount() const {
    return items_.size();
}

collision_detector::Item Game::Provider::GetItem(size_t index) const {
    return items_.at(index);
}

size_t Game::Provider::GatherersCount() const {
    return gatherers_.size();
}

collision_detector::Gatherer Game::Provider::GetGatherer(size_t index) const {
    return gatherers_.at(index);
}

// ============================================================
// HandleGatherAndReturn
// ============================================================
void Game::HandleGatherAndReturn(
    const std::unordered_map<uint32_t, geom::Point2D>& start_positions) {

    if (players_.empty()) {
        return;
    }
    const auto map_id = GetPlayerMap(players_.begin()->first);
    const Map* map_ptr = FindMap(map_id);
    if (!map_ptr) {
        return;
    }
    const Map& map = *map_ptr;

    Provider provider;

    std::vector<LostObjectId> item_ids;
    item_ids.reserve(lost_objects_.Size());
    for (const auto& [id, obj] : lost_objects_.Get()) {
        collision_detector::Item item;
        item.position = obj.pos;
        item.width = 0.0;
        provider.AddItem(item);
        item_ids.push_back(id);
    }

    std::vector<uint32_t> player_ids;
    player_ids.reserve(players_.size());
    for (const auto& [id, player] : players_) {
        const auto start_position = start_positions.at(id);
        const auto end_position = player.GetDog()->GetPosition();
        collision_detector::Gatherer gatherer;
        gatherer.start_pos = start_position;
        gatherer.end_pos = end_position;
        gatherer.width = 0.3;
        provider.AddGatherer(gatherer);
        player_ids.push_back(id);
    }

    std::vector<TickEvent> events;

    if (!player_ids.empty() && !item_ids.empty()) {
        auto collects = collision_detector::FindGatherEvents(provider);
        for (const auto& collect : collects) {
            TickEvent event;
            event.time = collect.time;
            event.type = TickEvent::Type::Collect;
            event.player_id = player_ids[collect.gatherer_id];
            event.item_id = item_ids[collect.item_id];
            events.push_back(event);
        }
    }

    for (const auto& office : map.GetOffices()) {
        const geom::Point2D office_pos{
            static_cast<double>(office.GetPosition().x),
            static_cast<double>(office.GetPosition().y)
        };
        for (std::size_t i = 0; i < player_ids.size(); ++i) {
            const auto start_position = start_positions.at(player_ids[i]);
            const auto end_position = players_.at(player_ids[i]).GetDog()->GetPosition();
            if (std::abs(end_position.x - start_position.x) < 1e-12 &&
                std::abs(end_position.y - start_position.y) < 1e-12) {
                continue;
            }
            auto result = collision_detector::TryCollectPoint(start_position,
                                                              end_position,
                                                              office_pos);
            constexpr double PLAYER_HALF = 0.3;
            constexpr double OFFICE_HALF = 0.25;
            if (result.IsCollected(PLAYER_HALF + OFFICE_HALF)) {
                TickEvent event;
                event.time = result.proj_ratio;
                event.type = TickEvent::Type::Return;
                event.player_id = player_ids[i];
                events.push_back(event);
            }
        }
    }

    std::sort(events.begin(), events.end(),
              [](const TickEvent& a, const TickEvent& b) { return a.time < b.time; });

    for (const auto& event : events) {
        auto player_it = players_.find(event.player_id);
        if (player_it == players_.end()) {
            continue;
        }
        auto& dog = player_it->second.GetDog();

        if (event.type == TickEvent::Type::Collect) {
            if (dog->IsBagFull()) {
                continue;
            }
            const LostObject* lost_object = lost_objects_.Find(event.item_id);
            if (!lost_object) {
                continue;
            }
            dog->PutToBag(FoundObject{FoundObject::Id{lost_object->id}, lost_object->type});
            lost_objects_.Remove(event.item_id);
        } else {
            for (const auto& item : dog->GetBagContent()) {
                dog->AddScore(static_cast<Score>(map.GetLootValue(item.type)));
            }
            dog->EmptyBag();
        }
    }
}

}  // namespace model
