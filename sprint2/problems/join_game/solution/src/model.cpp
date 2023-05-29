#include "model.h"

#include <stdexcept>

#include <iostream>

#include <utility>

namespace model {
using namespace std::literals;

int Dog::dog_count = 0;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

void Game::AddGameSession(Map::Id id) {
    const size_t index = sessions_.size();
    if (auto [it, inserted] = game_session_id_to_index_.emplace(id, index); !inserted) {
        throw std::invalid_argument("Session with id "s + *id + " already exists"s);
    } else {
        try{
            const model::Map* map_ptr = FindMap(util::Tagged<std::string, model::Map>(id));
            GameSession session(map_ptr);
            sessions_.emplace_back(std::move(session));
            std::cout << "session added\n";
        }
        catch (...)
        {
            throw;
        }
    }
}

Dog* GameSession::AddDog(std::string name)
{
    Dog* ret = nullptr;
    const size_t index = dogs_.size();
    try{
        dogs_.emplace_back(Dog{name});
        ret = &dogs_[index];
    }
    catch(...){
        throw;
    }
    return ret;
}

std::pair<std::string, Player*> Players::AddPlayer(GameSession* session ,Dog* dog_ptr)
{
    PlayerTokens generator;
    const size_t index = players_.size();
    Player& pl = players_.emplace_back(Player{session, dog_ptr});
    try {
        auto it = players_map.emplace(generator.generate_token(), &pl);
        return *it.first;
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        players_.pop_back();
        throw;
    }

    return {0,0};

}


Player* Players::FindByToken(std::string token)
{
    Player* ret;
    try
    {
        ret = players_map.at(token);
    }
    catch(std::out_of_range&)
    {
        ret = nullptr;
    }
    return ret;
}



}  // namespace model
