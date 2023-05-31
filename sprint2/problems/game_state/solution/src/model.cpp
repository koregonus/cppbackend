#include "model.h"

#include <stdexcept>

#include <iostream>

#include <utility>

#include <random>

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
            // GameSession* session = new GameSession(map_ptr);
            sessions_.emplace_back(GameSession{map_ptr});
            // std::cout << "session added\n";
        }
        catch (...)
        {
            throw;
        }
    }
}

Dog* GameSession::AddDog(std::string name, double x, double y)
{
    Dog* ret = nullptr;
    const size_t index = dogs_.size();
    try{
        dogs_.emplace_back(Dog{name, x, y});
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
    std::string token = generator.generate_token();

    const size_t index = players_.size();
    // std::cout << "index: " << index << std:: endl;

    Player* player = new Player(session, dog_ptr);

    players_.push_back(player);

    // std::cout << "player ptr " << std::hex << (uint64_t)&players_[index] << std::endl;

    // std::cout << "session ptr add:: " << std::hex << (uint64_t)session << std::endl;
    
    try {
        auto it = players_map.emplace(std::move(token), player);
        // Проверка на добавление в мапу, обработки нет поэтому пока отключена.
        // if(it.second == true)
        //     std::cout << "OK\n";
        return *it.first;
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        players_.pop_back();
        throw;
    }
    // }
    
    

    return {0,0};

}


Player* Players::FindByToken(std::string token)
{
    Player* ret;
    try
    {
        ret = players_map.at(token);
    }
    catch(...)
    {
        ret = nullptr;
    }
    return ret;
}

std::string PlayerTokens::generate_token()
    {
        std::stringstream stream;
        stream << std::hex << generator1_() << generator2_() << generator1_();
        std::string result( stream.str() );
        size_t len =  48 - result.length();
        if(len != 0)
        {
            std::cout << "wrong! ::" << len << std::endl;
        //     stream << std::hex << generator1_();
        //     std::string buf(stream.str());
        //     result.append(buf.substr(len));
        }
        result.resize(32);
        return result;
    }



}  // namespace model
