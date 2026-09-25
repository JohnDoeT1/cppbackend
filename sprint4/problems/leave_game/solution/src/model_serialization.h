#pragma once

#include <boost/serialization/optional.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>

#include "model.h"

// ============================================================
// serialize для util::Tagged<Value, Tag>
// ============================================================
namespace util {

template <typename Archive, typename Value, typename Tag>
void serialize(Archive& ar, Tagged<Value, Tag>& tag, [[maybe_unused]] unsigned version) {
    ar&* tag;
}

}  // namespace util

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& p, [[maybe_unused]] unsigned version) {
    ar& p.x;
    ar& p.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& v, [[maybe_unused]] unsigned version) {
    ar& v.x;
    ar& v.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, FoundObject& o, [[maybe_unused]] unsigned version) {
    ar&* o.id;
    ar& o.type;
}

template <typename Archive>
void serialize(Archive& ar, LostObject& o, [[maybe_unused]] unsigned version) {
    ar& o.id;
    ar& o.type;
    ar& o.value;
    ar& o.pos;
}

template <typename Archive>
void serialize(Archive& ar, Direction& d, [[maybe_unused]] unsigned version) {
    int v = static_cast<int>(d);
    ar& v;
    if (Archive::is_loading::value) {
        d = static_cast<Direction>(v);
    }
}

}  // namespace model

namespace serialization {

// ============================================================
// DogRepr
// ============================================================
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPosition())
        , bag_capacity_(dog.GetBagCapacity())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDirection())
        , score_(dog.GetScore())
        , bag_content_(dog.GetBagContent()) {}

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{id_, name_, pos_, bag_capacity_};
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.AddScore(score_);
        for (const auto& item : bag_content_) {
            if (!dog.PutToBag(item)) {
                throw std::runtime_error("Failed to restore dog bag");
            }
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] unsigned version) {
        ar&* id_;
        ar& name_;
        ar& pos_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:
    model::Dog::Id id_{0u};
    std::string name_;
    geom::Point2D pos_;
    std::size_t bag_capacity_ = 0;
    geom::Vec2D speed_;
    model::Direction direction_ = model::Direction::NORTH;
    model::Score score_ = 0;
    model::Dog::BagContent bag_content_;
};

// ============================================================
// PlayerRepr — теперь сохраняем времена активности
// ============================================================
class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const model::Player& p)
        : id_(p.GetId())
        , dog_(p.GetDog() ? DogRepr{*p.GetDog()} : DogRepr{})
        , join_time_ms_(p.GetJoinTimeMs())
        , last_active_time_ms_(p.GetLastActiveTimeMs()) {}

    [[nodiscard]] model::Player Restore() const {
        auto dog = std::make_shared<model::Dog>(dog_.Restore());
        model::Player p{id_, std::move(dog)};
        p.SetJoinTimeMs(join_time_ms_);
        p.SetLastActiveTimeMs(last_active_time_ms_);
        return p;
    }

    uint32_t GetId() const noexcept { return id_; }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] unsigned version) {
        ar& id_;
        ar& dog_;
        ar& join_time_ms_;
        ar& last_active_time_ms_;
    }

private:
    uint32_t id_ = 0;
    DogRepr dog_;
    int64_t join_time_ms_ = 0;
    int64_t last_active_time_ms_ = 0;
};

// ============================================================
// LostObjectRepr
// ============================================================
class LostObjectRepr {
public:
    LostObjectRepr() = default;
    explicit LostObjectRepr(const model::LostObject& o) : obj_(o) {}
    const model::LostObject& Get() const noexcept { return obj_; }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] unsigned version) {
        ar& obj_;
    }

private:
    model::LostObject obj_;
};

// ============================================================
// MapPlayersEntry
// ============================================================
struct MapPlayersEntry {
    model::Map::Id map_id{std::string{}};
    std::vector<uint32_t> player_ids;

    MapPlayersEntry() = default;
    MapPlayersEntry(model::Map::Id id, std::vector<uint32_t> ids)
        : map_id(std::move(id)), player_ids(std::move(ids)) {}

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] unsigned version) {
        ar& map_id;
        ar& player_ids;
    }
};

// ============================================================
// TokenEntry
// ============================================================
struct TokenEntry {
    std::string token;
    uint32_t player_id = 0;

    TokenEntry() = default;
    TokenEntry(std::string t, uint32_t id) : token(std::move(t)), player_id(id) {}

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] unsigned version) {
        ar& token;
        ar& player_id;
    }
};

// ============================================================
// GameStateRepr
// ============================================================
class GameStateRepr {
public:
    GameStateRepr() = default;

    std::vector<PlayerRepr> players;
    std::vector<MapPlayersEntry> map_players;
    std::vector<TokenEntry> tokens;
    std::vector<LostObjectRepr> lost_objects;
    uint32_t next_player_id = 0;
    model::LostObjectId next_lost_object_id = 0;
    int64_t elapsed_ms = 0;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] unsigned version) {
        ar& players;
        ar& map_players;
        ar& tokens;
        ar& lost_objects;
        ar& next_player_id;
        ar& next_lost_object_id;
        ar& elapsed_ms;
    }
};

}  // namespace serialization
