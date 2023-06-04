#include "model.h"

#include <stdexcept>

#include <iostream>

#include <utility>

#include <random>

namespace model {
using namespace std::literals;

int Dog::dog_count = 0;


bool Road::PointIsWithinRoad(double x, double y) const
{
    bool check_y = false;
    
    
    if(end_.x >= start_.x)
    {
        // std::cout << "1\n";
        // std::cout << "X start:" << x << ">=" << ((double)(start_.x)) - 4E-1 << std::endl;
        // std::cout << "X end:" << x << "<=" << ((double)(end_.x)) + 4E-1 << std::endl;
        // std::cout << fabs(x-start_.x - 4E-1) << std::endl;
        // std::cout << fabs(x-end_.x + 4E-1) << std::endl;
        if(!((x >= (double(start_.x) - 4E-1)) && (x <= (double(end_.x) + 4E-1))))
            return false;
    }
    else
    {
        // std::cout << "2\n";
        // std::cout << "X start:" << x << "<=" << ((double)(start_.x)) + 4E-1 << std::endl;
        // std::cout << "X end:" << x << ">=" << ((double)(end_.x)) - 4E-1 << std::endl;
        // std::cout << fabs(x-start_.x) << std::endl;
        // std::cout << fabs(x-end_.x-0.4) << std::endl;
        // if(!((x < (double(start_.x) + 0.4f) || fabs(x-start_.x) < 1E-9) && 
        //     (x >= (double(end_.x) - 0.4f)) || (fabs(x-end_.x) < 1E-9)))
        if(!((x <= (double(start_.x) - 4E-1)) && (x >= (double(end_.x) + 4E-1))))
            return false;
    }
    if(end_.y >= start_.y)
    {
        // std::cout << "3\n";
        // std::cout << "Y start:" << y << ">=" << ((double)(start_.y)) - 0.4 << std::endl;
        // std::cout << "Y end:" << y << "<=" << ((double)(end_.y)) + 0.4 << std::endl;
        if(!(y >= ((double)(start_.y)) - 4E-1 && (y <= ((double)(end_.y) + 4E-1))))
            return false;
    }
    else
    {
        // std::cout << "4\n";
        // std::cout << "Y start:" << y << "<=" << ((double)(start_.y)) + 0.4 << std::endl;
        // std::cout << "Y end:" << y << ">=" << ((double)(end_.y)) - 0.4 << std::endl;
        if(!(y <= ((double)(start_.y) + 4E-1) && (y >= ((double)(end_.y) - 4E-1))))
            return false;
    }
    return true;
}


void Map::AddRoad(const Road& road)
{
    size_t idx = roads_.size();

    roads_.emplace_back(road);
    if(road.IsHorizontal())
        horizontal_roads_idx.push_back(idx);
    else
        vertical_roads_idxs.push_back(idx);
}


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

void GameSession::UpdateDogs(double tick_ms)
{

    for(auto it = dogs_.begin(); it < dogs_.end(); it++)
    {
        // std::cout << "update dogs\n";
        it->Update(tick_ms);
    }
}

void Game::UpdateSessionsTime(double tick_ms)
{
    // std::cout << "ses size " << sessions_.size() << std:: endl;
    if(sessions_.size() == 0)
        return;
    for(auto it = sessions_.begin(); it < sessions_.end(); it++)
    {
        // std::cout << "update ses\n";
        it->UpdateDogs(tick_ms);
    }
}

Dog* GameSession::AddDog(std::string name, double x, double y, const model::Map* map_ptr, int idx)
{
    Dog* ret = nullptr;
    const size_t index = dogs_.size();
    try{
        dogs_.emplace_back(Dog{name, x, y, map_ptr, idx});
        ret = &dogs_[index];
    }
    catch(...){
        throw;
    }
    return ret;
}



void Dog::Update(double tick_ms)
{
    if(coords_.direction == 4)
        return;
    // std::cout << "update one doggy\n";
    double dx = coords_.vx * (tick_ms)/1000.0;
    double dy = coords_.vy * (tick_ms)/1000.0;

    double next_p_x = coords_.x + dx;
    double next_p_y = coords_.y + dy;
    auto roads = map_ptr_->GetRoads();
    // std::cout << "cur road ptr " << (uint64_t)current_road << std::endl;
    // std::cout << "next dx coords :" << dx << " " << dy << std::endl;
    // std::cout << "coords :" << coords_.x << " " << coords_.y << std::endl;
    // std::cout << "next coords :" << next_p_x << " " << next_p_y << std::endl;
    bool next_road_found = false;
    if(roads[current_road_idx].PointIsWithinRoad(next_p_x, next_p_y))
    {
        // std::cout << "Within\n";
        coords_.x = next_p_x;
        coords_.y = next_p_y;
        next_road_found = true;
    }
    else
    {
        if(coords_.direction == 2 || coords_.direction == 3)
        {
            // std::cout << "check hor roads\n";
            auto hor_roads = map_ptr_->GetHorRoads();
            // std::cout << " num ::" << hor_roads.size() << std::endl;
            for(auto it = hor_roads.begin(); it < hor_roads.end(); it++)
            {
                if(*it == current_road_idx)
                    continue;
                if(roads[*it].PointIsWithinRoad(next_p_x, next_p_y))
                {
                    coords_.x = next_p_x;
                    coords_.y = next_p_y;
                    current_road_idx = *it;
                    next_road_found = true;
                    break;
                }
            }
        }
        else if(coords_.direction == 0 || coords_.direction == 1)
        {
            // std::cout << "check ver roads\n";
            auto ver_roads = map_ptr_->GetVerRoads();
            // std::cout << " num ::" << ver_roads.size() << std::endl;
            for(auto it = ver_roads.begin(); it < ver_roads.end(); it++)
            {
                if(*it == current_road_idx)
                    continue;
                if(roads[*it].PointIsWithinRoad(next_p_x, next_p_y))
                {
                    coords_.x = next_p_x;
                    coords_.y = next_p_y;
                    current_road_idx = *it;
                    next_road_found = true;
                    break;
                }
            }
        }
    }
    if(!next_road_found)
    {
        std::cout << "road not found\n";
        if(coords_.direction == 0)
        {
            // std::cout << "UpBound\n";
            coords_.y = roads[current_road_idx].UpBound();
        }
        else if(coords_.direction == 1)
        {
            // std::cout << "LowBound\n";
            coords_.y = roads[current_road_idx].LowBound();
        }
        else if(coords_.direction == 2)
        {
            // std::cout << "LeftBound\n";
            coords_.x = roads[current_road_idx].LeftBound();
        }
        else if(coords_.direction == 3)
        {
            // std::cout << "RightBound\n";
            coords_.x = roads[current_road_idx].RightBound();
        }
    }
    std::cout << "current idx::" << current_road_idx << std:: endl;
    std::cout << "next coords real:" << coords_.x << " " << coords_.y << std::endl;
}

std::pair<std::string, Player*> Players::AddPlayer(GameSession* session ,Dog* dog_ptr)
{
    PlayerTokens generator;
    std::string token = generator.generate_token();

    const size_t index = players_.size();
    // std::cout << "index: " << index << std:: endl;

    Player* player = new Player(session, dog_ptr);

    players_.push_back(player);

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
        result.resize(32);
        return result;
    }



}  // namespace model
