#pragma once

#include <string>
#include <vector>

#include <pqxx/pqxx>

#include "connection_pool.h"

namespace infrastructure {

struct RetiredPlayerRecord {
    std::string name;
    int score = 0;
    double play_time = 0.0;
};

class RetiredPlayersRepository {
public:
    explicit RetiredPlayersRepository(ConnectionPool& pool) : pool_(pool) {}

    void EnsureSchema() {
        auto conn = pool_.GetConnection();
        pqxx::work tx{*conn};
        tx.exec(R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                id SERIAL PRIMARY KEY,
                name TEXT NOT NULL,
                score INTEGER NOT NULL,
                play_time DOUBLE PRECISION NOT NULL
            );
        )");
        tx.exec(R"(
            CREATE INDEX IF NOT EXISTS idx_retired_players_sort
                ON retired_players (score DESC, play_time ASC, name ASC);
        )");
        tx.commit();
    }

    void Save(const std::string& name, int score, double play_time) {
        auto conn = pool_.GetConnection();
        pqxx::work tx{*conn};
        tx.exec_params(
            "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3)",
            name, score, play_time);
        tx.commit();
    }

    std::vector<RetiredPlayerRecord> GetRecords(int start, int max_items) {
        auto conn = pool_.GetConnection();
        pqxx::read_transaction tx{*conn};
        auto rows = tx.exec_params(
            "SELECT name, score, play_time FROM retired_players "
            "ORDER BY score DESC, play_time ASC, name ASC "
            "OFFSET $1 LIMIT $2",
            start, max_items);

        std::vector<RetiredPlayerRecord> result;
        result.reserve(rows.size());
        for (const auto& row : rows) {
            RetiredPlayerRecord r;
            r.name = row["name"].as<std::string>();
            r.score = row["score"].as<int>();
            r.play_time = row["play_time"].as<double>();
            result.push_back(std::move(r));
        }
        return result;
    }

private:
    ConnectionPool& pool_;
};

}  // namespace infrastructure
