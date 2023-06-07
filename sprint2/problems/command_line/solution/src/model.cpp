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
        if(!((x >= (static_cast<double>(start_.x) - DELTA_ROAD_SIZE_PARAMETER)) && (x <= (static_cast<double>(end_.x) + DELTA_ROAD_SIZE_PARAMETER))))
        {
            return false;
        }
    }
    else
    {
        if(!((x <= (static_cast<double>(start_.x) - DELTA_ROAD_SIZE_PARAMETER)) && (x >= (static_cast<double>(end_.x) + DELTA_ROAD_SIZE_PARAMETER))))
        {
            return false;
        }
    }
    if(end_.y >= start_.y)
    {
        if(!((y >= (static_cast<double>(start_.y) - DELTA_ROAD_SIZE_PARAMETER) && (y <= (static_cast<double>(end_.y) + DELTA_ROAD_SIZE_PARAMETER)))))
        {
            return false;
        }
    }
    else
    {
        if(!(y <= (static_cast<double>(start_.y) + DELTA_ROAD_SIZE_PARAMETER) && (y >= (static_cast<double>(end_.y) - DELTA_ROAD_SIZE_PARAMETER))))
        {
            return false;
        }
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
            sessions_.emplace_back(GameSession{map_ptr});
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
        (*it)->Update(tick_ms);
    }
}

void Game::UpdateSessionsTime(double tick_ms)
{
    if(sessions_.size() == 0)
        return;
    for(auto it = sessions_.begin(); it < sessions_.end(); it++)
    {
        (*it).UpdateDogs(tick_ms);
    }
}

Dog* GameSession::AddDog(std::string name, double x, double y, const model::Map* map_ptr, int idx)
{
    const size_t index = dogs_.size();
    Dog* ret = new Dog(name, x, y, map_ptr, idx);
    try{
        dogs_.push_back(ret);
        ret = dogs_[index];
    }
    catch(...){
        throw;
    }
    return ret;
}



void Dog::Update(double tick_ms)
{
    if(coords_.direction == DogDirection::DOG_MOVE_STOP)
        return;
    int lag_count = 0;
    double tick_buff = 0.0;
    double tick_tail = 0.0;
    if(tick_ms > 5 * LAG_STEP)
    {
        lag_count = static_cast<int>(tick_ms / LAG_STEP);
        tick_buff = LAG_STEP;
        tick_tail = tick_ms - LAG_STEP*lag_count;
    }
    else
    {
        tick_buff = tick_ms;
        lag_count = 1;
    }
    
    for(int i = 0; i <= lag_count; i++)
    {
        if(i == lag_count && i != 1)
        {
            tick_buff = tick_tail;
        }
        else if(lag_count == 1 && i == 1)
        {
            break;
        }

        double dx = coords_.vx * (tick_buff)/1000.0; // tick_buff from millisec to sec
        double dy = coords_.vy * (tick_buff)/1000.0; // tick_buff from millisec to sec

        double next_p_x = coords_.x + dx;
        double next_p_y = coords_.y + dy;
        auto roads = map_ptr_->GetRoads();

        bool next_road_found = false;
        if(roads[current_road_idx].PointIsWithinRoad(next_p_x, next_p_y))
        {
            coords_.x = next_p_x;
            coords_.y = next_p_y;
            next_road_found = true;
        }
        else
        {
            if(coords_.direction == DogDirection::DOG_MOVE_LEFT || coords_.direction == DogDirection::DOG_MOVE_RIGHT)
            {
                auto hor_roads = map_ptr_->GetHorRoads();
                for(auto it = hor_roads.begin(); it < hor_roads.end(); it++)
                {
                    if(*it == current_road_idx)
                    {
                        continue;
                    }
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
            else if(coords_.direction == DogDirection::DOG_MOVE_UP || coords_.direction == DogDirection::DOG_MOVE_DOWN)
            {
                auto ver_roads = map_ptr_->GetVerRoads();
                for(auto it = ver_roads.begin(); it < ver_roads.end(); it++)
                {
                    if(*it == current_road_idx)
                    {
                        continue;
                    }
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
            if(coords_.direction == DogDirection::DOG_MOVE_UP)
            {
                coords_.y = roads[current_road_idx].UpBound();
            }
            else if(coords_.direction == DogDirection::DOG_MOVE_DOWN)
            {
                coords_.y = roads[current_road_idx].LowBound();
            }
            else if(coords_.direction == DogDirection::DOG_MOVE_LEFT)
            {
                coords_.x = roads[current_road_idx].LeftBound();
            }
            else if(coords_.direction == DogDirection::DOG_MOVE_RIGHT)
            {
                coords_.x = roads[current_road_idx].RightBound();
            }
            coords_.vx = 0.0;
            coords_.vy = 0.0;
        }
    }
}

std::pair<std::string, Player*> Players::AddPlayer(GameSession* session ,Dog* dog_ptr)
{
    PlayerTokens generator;
    std::string token = generator.generate_token();

    const size_t index = players_.size();

    Player* player = new Player(session, dog_ptr);

    players_.push_back(player);

    try {
        auto it = players_map.emplace(std::move(token), player);
        return *it.first;
    } catch (...) {
        // Удаляем игрока из вектора, если не удалось вставить в вектор
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
