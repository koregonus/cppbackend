#include "model.h"

#include <stdexcept>

#include <iostream>

#include <utility>

#include <random>

#include "collision_detector.h"

namespace model {
using namespace std::literals;


double model_random_generator()
{
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
}

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
    {
        horizontal_roads_idx.push_back(idx);
    }
    else
    {
        vertical_roads_idxs.push_back(idx);
    }
}


void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId()))
    {
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

unsigned Game::GetNumberOfItemsToAdd(std::chrono::milliseconds time_delta, unsigned loot_count, unsigned looter_count) const
{
    return loot_g_->Generate(time_delta, loot_count, looter_count);
}

void GameSession::UpdateDogs(double tick_ms)
{

    for(auto it = dogs_.begin(); it < dogs_.end(); it++)
    {
        (*it)->Update(tick_ms);
    }
}

std::pair<double, double> GameSession::GetRandomPointOnMap()
{
    auto roads = map_ptr_->GetRoads();
    std::chrono::system_clock::time_point cur_ts = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point time_point_nano = std::chrono::time_point_cast<std::chrono::nanoseconds>(cur_ts);
    int seed = time_point_nano.time_since_epoch().count();
    std::srand(seed);
    int del = (roads.size());
    if(del == 0)
        del = 1;
    int road_idx = 0;
    double x_d, y_d;
                   

    road_idx = std::rand() % del;
    model::Point start_p = roads[road_idx].GetStart();
    model::Point end_p = roads[road_idx].GetEnd();
    if(roads[road_idx].IsHorizontal())
    {
        x_d = static_cast<double>((start_p.x > end_p.x) ?
        (end_p.x + std::rand() % abs(start_p.x - end_p.x)):
        (start_p.x + std::rand() % abs(start_p.x - end_p.x)));
        y_d = static_cast<double>(start_p.y);
    }
    else
    {
        y_d = static_cast<double>((start_p.y > end_p.y) ?
        (end_p.y + std::rand() % abs(start_p.y - end_p.y)):
        (start_p.y + std::rand() % abs(start_p.y - end_p.y)));
        x_d = static_cast<double>(start_p.x);
    }

    std::pair<double, double> ret{x_d, y_d};
    return ret;
}

void Game::UpdateSessionsTime(std::chrono::milliseconds tick_ms)
{
    double make_tick = (double)(tick_ms.count());
    if(sessions_.size() == 0)
    {
        return;
    }
    for(auto it = sessions_.begin(); it < sessions_.end(); it++)
    {
        auto dogs = it->GetDogs();
        auto loot_objs = it->GetLootObjs();
        int base_count = it->GetCountOfBases();
        unsigned count = loot_g_->Generate(tick_ms, loot_objs.size() - base_count, dogs.size());
        auto map_ptr = it->GetMap();
        auto rand_types = map_ptr->GetLootObjsCount();
        for(int i = 0; i < count; i++)
        {
            std::chrono::system_clock::time_point cur_ts = std::chrono::system_clock::now();
            std::chrono::system_clock::time_point time_point_nano = std::chrono::time_point_cast<std::chrono::nanoseconds>(cur_ts);
            int seed = time_point_nano.time_since_epoch().count();
            auto coords_for_loot = it->GetRandomPointOnMap();
            std::srand(seed);
            auto type = rand()%rand_types;
            it->AddLootObj(type, coords_for_loot.first, coords_for_loot.second, ITEM_WIDTH, map_ptr->GetLootValue(type), false);
        }
        (*it).UpdateDogs(make_tick);
        auto finded_gather_elm = collision_detector::FindGatherEvents((*it));
        if(finded_gather_elm.size() == 0)
        {
            continue;
        }
        else
        {
            auto map_cap = map_ptr->GetBagCapacity();
            std::vector<int> processed_items = {};
            processed_items.resize(finded_gather_elm.size());
            for( auto find_gat_it = finded_gather_elm.begin(); find_gat_it != finded_gather_elm.end(); find_gat_it++)
            {
                if(loot_objs[find_gat_it->item_id]->type_  == LOOT_TYPE_BASE)
                {
                    // clear bag logic
                    dogs[find_gat_it->gatherer_id]->ClearLootBag();
                    continue;
                }
                else
                {
                    // check loot bag
                    auto dog_bag_cap = dogs[find_gat_it->gatherer_id]->GetLootBagSize();
                    if(dog_bag_cap >= map_cap)
                    {
                        continue;
                    }

                    // check if item already processed
                    bool alredyProcessed = false;
                    for(auto it_local_proc = processed_items.begin(); it_local_proc != processed_items.end(); it_local_proc++)
                    {
                        if(*it_local_proc == find_gat_it->item_id)
                            alredyProcessed = true;
                    }
                    
                    // process item:
                    if(alredyProcessed)
                    {
                        continue;
                    }
                    else
                    {
                        // append to loot bag
                        dogs[find_gat_it->gatherer_id]->AddToLootBag(find_gat_it->item_id, 
                                                                    loot_objs[find_gat_it->item_id]->type_);
                        // append to processed list
                        processed_items.push_back(find_gat_it->item_id);
                        // erase processed element
                        it->EraseLootObj(find_gat_it->item_id);
                    }
                }
            }
        }

    }
}

std::shared_ptr<Dog> GameSession::AddDog(std::string name, double x, double y, const model::Map* map_ptr, int idx)
{
    const size_t index = dogs_.size();
    std::shared_ptr<Dog> ret = std::make_shared<Dog>(name, x, y, map_ptr, idx);
    try{
        dogs_.push_back(ret);
        ret = dogs_[index];
    }
    catch(...){
        throw;
    }
    return ret;
}

std::shared_ptr<Dog> GameSession::AddDog(std::shared_ptr<Dog> restored_doggy)
{
    const size_t index = dogs_.size();
    try{
        dogs_.push_back(restored_doggy);
    }
    catch(...){
        throw;
    }
    return restored_doggy;
}



void Dog::Update(double tick_ms)
{
    if(coords_.direction == DogDirection::DOG_MOVE_STOP)
    {
        return;
    }
    int lag_count = 0;
    double tick_buff = 0.0;
    double tick_tail = 0.0;
    prev_x = coords_.x;
    prev_y = coords_.y;
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

std::pair<std::string, std::shared_ptr<Player>> Players::AddPlayer(GameSession* session ,std::shared_ptr<Dog> dog_ptr)
{
    PlayerTokens generator;
    std::string token = generator.generate_token();

    const size_t index = players_.size();

    std::shared_ptr<Player> player = std::make_shared<Player>(session, dog_ptr, token);

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

std::pair<std::string, std::shared_ptr<Player>> Players::AddPlayer(GameSession* session ,std::shared_ptr<Dog> dog_ptr, std::string token_restore)
{
    const size_t index = players_.size();

    std::shared_ptr<Player> player = std::make_shared<Player>(session, dog_ptr, token_restore);

    players_.push_back(player);

    try {
        auto it = players_map.emplace(std::move(token_restore), player);
        return *it.first;
    } catch (...) {
        // Удаляем игрока из вектора, если не удалось вставить в вектор
        players_.pop_back();
        throw;
    }    

    return {0,0};

}


std::optional<std::shared_ptr<Player>> Players::FindByToken(std::string token)
{
    std::optional<std::shared_ptr<Player>> ret;
    try
    {
        ret = players_map.at(token);
    }
    catch(...)
    {
        ret = std::nullopt;
    }
    return ret;
}

std::optional<std::shared_ptr<Player>> Players::FindByDogId(model::Dog::Id dog_id)
{
    for(auto iter = players_.begin(); iter != players_.end(); iter++)
    {
        if(dog_id == (*iter)->GetId())
            return *iter;
    }
    return std::nullopt;
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
