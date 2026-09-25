#pragma once

#include "model.h"
#include "retired_players_repository.h"

namespace infrastructure {

class RetirementHandler : public model::RetirementListener {
public:
    explicit RetirementHandler(RetiredPlayersRepository& repo) : repo_(repo) {}

    void OnPlayerRetired(const std::string& name,
                         int score,
                         double play_time_sec) override {
        try {
            repo_.Save(name, score, play_time_sec);
        } catch (const std::exception&) {
            // Логирование не критично — уходит в /dev/null
        }
    }

private:
    RetiredPlayersRepository& repo_;
};

}  // namespace infrastructure
