#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <stdexcept>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "model.h"
#include "model_serialization.h"

namespace infrastructure {

class SerializingListener {
public:
    using milliseconds = std::chrono::milliseconds;

    SerializingListener(model::Game& game,
                        std::filesystem::path state_file,
                        std::optional<milliseconds> save_period)
        : game_(game)
        , state_file_(std::move(state_file))
        , save_period_(save_period) {}

    // Синхронизация накопленного времени после восстановления
    void SetElapsed(milliseconds t) noexcept {
        elapsed_ = t;
        last_save_ = t;   // чтобы первое автосохранение не сработало сразу
    }

    // Вызывается из Game::Tick через хук
    void OnTick(milliseconds delta) {
        elapsed_ += delta;
        if (save_period_ && elapsed_ - last_save_ >= *save_period_) {
            Save();
            last_save_ = elapsed_;
        }
    }

    void Save() {
        std::lock_guard lock(mutex_);

        auto tmp = state_file_;
        tmp += ".tmp";

        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            if (!out) {
                throw std::runtime_error("Cannot open tmp state file: " + tmp.string());
            }

            auto repr = Serialize(game_);
            boost::archive::text_oarchive oa(out);
            oa << repr;
            out.flush();
            if (!out) {
                throw std::runtime_error("Failed to write state file: " + tmp.string());
            }
        }

        std::filesystem::rename(tmp, state_file_);
    }

    static void Load(model::Game& game, const std::filesystem::path& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Cannot open state file: " + path.string());
        }

        serialization::GameStateRepr repr;
        boost::archive::text_iarchive ia(in);
        ia >> repr;

        Restore(game, repr);
    }

private:
    static serialization::GameStateRepr Serialize(const model::Game& game) {
        serialization::GameStateRepr repr;

        for (const auto& [id, player] : game.GetPlayers()) {
            repr.players.emplace_back(player);
        }
        for (const auto& [map_id, ids] : game.GetMapPlayers()) {
            repr.map_players.emplace_back(map_id, ids);
        }
        for (const auto& [token, pid] : game.GetTokenToPlayer()) {
            repr.tokens.emplace_back(token, pid);
        }
        for (const auto& [id, obj] : game.GetLostObjects().Get()) {
            repr.lost_objects.emplace_back(obj);
        }
        repr.next_player_id = game.GetNextPlayerId();
        repr.next_lost_object_id = game.GetLostObjects().GetNextId();
        repr.elapsed_ms = game.GetElapsedMs();

        return repr;
    }

    static void Restore(model::Game& game, const serialization::GameStateRepr& repr) {
        model::Game::Players players;
        for (const auto& p : repr.players) {
            players.emplace(p.GetId(), p.Restore());
        }
        game.SetPlayers(std::move(players));

        model::Game::MapPlayers mp;
        for (const auto& [map_id, ids] : repr.map_players) mp[map_id] = ids;
        game.SetMapPlayers(std::move(mp));

        model::Game::TokenToPlayerId tokens;
        std::unordered_map<uint32_t, std::string> player_to_token;
        for (const auto& [token, pid] : repr.tokens) {
            tokens[token] = pid;
            player_to_token[pid] = token;
        }
        game.SetTokenToPlayer(std::move(tokens));
        game.SetPlayerToToken(std::move(player_to_token));

        model::LostObjects::Storage storage;
        for (const auto& lo : repr.lost_objects) {
            const auto& obj = lo.Get();
            storage.emplace(obj.id, obj);
        }
        game.GetLostObjects().SetStorage(std::move(storage), repr.next_lost_object_id);

        game.SetNextPlayerId(repr.next_player_id);
        game.SetElapsedMs(repr.elapsed_ms);
    }

    model::Game& game_;
    std::filesystem::path state_file_;
    std::optional<milliseconds> save_period_;
    milliseconds elapsed_{0};
    milliseconds last_save_{0};
    std::mutex mutex_;
};

}  // namespace infrastructure
