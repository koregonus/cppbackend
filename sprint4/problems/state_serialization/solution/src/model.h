#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include <random>

#include "tagged.h"

#include <chrono>

#include <map>

#include <iomanip>

#include <memory>

#include <iostream>

#include <optional>

#include "application_support.h"

#include "collision_detector.h"

#include "geom.h"

namespace model {

using Dimension = int;
using Coord = Dimension;
using Score = int;

const std::map<std::string, int> dog_direction_map {{"NORTH", 0},
                                                   {"SOUTH", 1},
                                                   {"WEST", 2},
                                                   {"EAST", 3}
                                                   };

// Вспомогательная структура описания направления движения псов
struct DogDirection {
    DogDirection() = delete;
    constexpr static int DOG_MOVE_UP = 0;
    constexpr static int DOG_MOVE_DOWN = 1;
    constexpr static int DOG_MOVE_LEFT = 2;
    constexpr static int DOG_MOVE_RIGHT = 3;
    constexpr static int DOG_MOVE_STOP = 4;
};


constexpr static double DELTA_ROAD_SIZE_PARAMETER = 4E-1;
constexpr static double LAG_STEP = 100.0; 
constexpr int LOOT_TYPE_BASE = -1; 
constexpr double PLAYER_WIDTH = 0.6;
constexpr double BASE_AKA_OFFICE_WIDTH = 0.5;
constexpr double ITEM_WIDTH = 0.0;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct Dog_mv_param{

    explicit Dog_mv_param(double x0, double y0):x(x0), y(y0), vx(0), vy(0),direction(0) {}

    double x;
    double y;
    double vx;
    double vy;
    int direction;
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

    double LeftBound() const noexcept {
        return (((double)(start_.x <= end_.x)?start_.x:end_.x) - DELTA_ROAD_SIZE_PARAMETER);
    }

    double RightBound() const noexcept{
        return (((double)(start_.x <= end_.x)?end_.x:start_.x) + DELTA_ROAD_SIZE_PARAMETER);
    }

    double UpBound() const noexcept{
        return (((double)(start_.y <= end_.y)?start_.y:end_.y) - DELTA_ROAD_SIZE_PARAMETER);
        
    }

    double LowBound() const noexcept{
        return (((double)(start_.y <= end_.y)?end_.y:start_.y) + DELTA_ROAD_SIZE_PARAMETER);
    }

    bool PointIsWithinRoad(double x, double y) const;
    



private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    const std::vector<int>& GetHorRoads() const noexcept{
        return horizontal_roads_idx;
    }


    const std::vector<int>& GetVerRoads() const noexcept{
        return vertical_roads_idxs;
    }

    void AddLootValue(int key_type, int value)
    {
        loot_value_map.insert(std::pair{key_type, value});
    }

    int GetLootValue(int key_type) const
    {
        int ret = 0;
        try
        {
            ret = loot_value_map.at(key_type);
        }
        catch(...)
        {
            ret = 0;
        }
        return ret;
    }

    void AddRoad(const Road& road);

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void SetDogSpeed(double NewDogSpeed)
    {
        DogSpeed = NewDogSpeed;
    }

    double GetDogSpeed() const
    {
        return DogSpeed;
    }

    void SetBagCapacity(uint64_t capacity)
    {
        BagCapacity = capacity;
    }

    size_t GetBagCapacity() const
    {
        return BagCapacity;
    }

    void AddOffice(Office office);

    unsigned GetLootObjsCount() const
    {
        return loot_objs_count_;
    }

    void SetLootObjsCount(unsigned count)
    {
        loot_objs_count_ = count;
    }


private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    std::vector<int> horizontal_roads_idx;
    std::vector<int> vertical_roads_idxs;
    std::map<int, int> loot_value_map;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double DogSpeed;
    size_t BagCapacity;
    unsigned loot_objs_count_;
};



namespace detail {
struct TokenTag {};
}  // namespace detail


class PlayerTokens {
public:
    PlayerTokens()
    {
        std::chrono::system_clock::time_point cur_ts = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point time_point_sec_1 = std::chrono::time_point_cast<std::chrono::milliseconds>(cur_ts);
        std::chrono::system_clock::time_point time_point_sec_2 = std::chrono::time_point_cast<std::chrono::nanoseconds>(cur_ts);
        int seed_1 = time_point_sec_1.time_since_epoch().count();
        int seed_2 = time_point_sec_2.time_since_epoch().count();

        generator1_.seed(seed_1^seed_2);
        generator2_.seed(seed_2+seed_1);
    }

    std::string generate_token();

private:
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    // Чтобы сгенерировать токен, получите из generator1_ и generator2_
    // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
    // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
    // чтобы сделать их подбор ещё более затруднительным
};

struct DoggyLoot
{
    int id;
    int type;
};

class Dog{
public:
    using Id = util::Tagged<int, Dog>;
    Dog(std::string name, double x, double y, const Map* map_ptr, int idx): dog_name{std::move(name)}, id_(dog_count++), coords_{x, y}, 
                                                                map_ptr_(map_ptr), current_road_idx(idx) {}
         
    Dog(Id id, std::string name, double x, double y, const Map* map_ptr, int idx): dog_name{std::move(name)}, id_(id), coords_{x, y}, 
                                                                map_ptr_(map_ptr), current_road_idx(idx) {}                                       

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return dog_name;
    }

    const std::vector<std::shared_ptr<DoggyLoot>>& GetLootBag() const noexcept
    {
        return LootBag;
    }

    const Dog_mv_param& GetDogParams() const noexcept {
        return coords_;
    }

    double GetWidth() const
    {
        return PLAYER_WIDTH;
    }

    std::pair<double, double> GetCurPos()
    {
        return std::pair<double, double>{coords_.x, coords_.y};
    }

    std::pair<double, double> GetPrevPos()
    {
        return std::pair<double, double>{prev_x, prev_y};
    }

    geom::Point2D GetPositionPoint() const
    {
        return geom::Point2D{coords_.x, coords_.y};
    }

    void SetDogOrientation(std::pair<int, int> basis, double speed, int dir)
    {
        coords_.vx = speed*basis.first;
        coords_.vy = speed*basis.second;
        coords_.direction = dir;
    }

    geom::Vec2D GetDogSpeedVec() const
    {
        return geom::Vec2D{coords_.vx, coords_.vy}; 
    }

    void SetDogSpeedVec(geom::Vec2D speed)
    {
        coords_.vx = speed.x;
        coords_.vy = speed.y;
    }

    void SetDogDirection(int direction)
    {
        coords_.direction = direction;
    }

    int GetDogDirection() const
    {
        return coords_.direction;
    }

    void Update(double tick_ms);

    size_t GetLootBagSize() const
    {
        return LootBag.size();
    }

    int GetRoadIdx() const
    {
        return current_road_idx;
    }

    int GetScore() const
    {
        return score;
    }

    void AddToLootBag(int id, int type)
    {
        LootBag.push_back(std::make_shared<DoggyLoot>(id,type));
    }

    void ClearLootBag()
    {
        for(auto it = LootBag.begin(); it != LootBag.end(); it++)
        {
            AddToScore(map_ptr_->GetLootValue((*it)->type));
        }
        LootBag.clear();
    }

    void TrySetDogCount(int count)
    {
        if(count >= dog_count)
            dog_count = count + 1;
    }

    void AddToScore(int add_score)
    {
        score += add_score;
    }

private:
    std::vector<std::shared_ptr<DoggyLoot>> LootBag;
    Dog_mv_param coords_;
    static int dog_count;
    size_t current_road_idx = 0;
    const Map* map_ptr_;
    std::string dog_name;
    double prev_x = 0;
    double prev_y = 0;
    Id id_;
    int score = 0;
    
};

struct LootObj
{
    LootObj() = default;
    LootObj(int type, double x, double y, double width, double value): type_(type), x_(x), y_(y), 
                                                                        width_(width), value_(value){}
    
    // LootObj(const LootObj& loot): type_(loot.type_), x_(loot.x_), y_(loot.y_), 
    //                         width_(loot.width_), value_(loot.value_){}
    int type_;
    double x_;
    double y_;
    double width_;
    int value_;
};



class GameSession: public collision_detector::ItemGathererProvider
{
    using Id = util::Tagged<std::string, Map>;
    using Dogs = std::vector<Dog>;
public:
    GameSession(const Map* m_ptr):map_ptr_(m_ptr){
        auto offices = map_ptr_->GetOffices();
        for(auto it = offices.begin(); it != offices.end();it++)
        {
            auto point = it->GetPosition();
            AddLootObj(LOOT_TYPE_BASE, static_cast<double>(point.x), static_cast<double>(point.y), BASE_AKA_OFFICE_WIDTH, 0, true);
        }
    }

    const Id& GetSessionMapId() const noexcept {
        return map_ptr_->GetId();
    }

    const std::vector<std::shared_ptr<Dog>>& GetDogs() const noexcept {
        return dogs_;
    }

    const Map* GetMap() const
    {
        return map_ptr_;
    }

    std::shared_ptr<Dog> AddDog(std::string name, double x, double y, const Map* road_ptr, int road_idx);

    std::shared_ptr<Dog> AddDog(std::shared_ptr<Dog> restored_doggy);


    void UpdateDogs(double tick_ms);

    void AddLootObj(int type, double x, double y, double width, double value, bool isBase)
    {
        loot_objs_.push_back(std::make_shared<LootObj>(type, x, y, width, value));
        if(isBase)
            count_base_++;
    }

    void EraseLootObj(int idx)
    {
        loot_objs_.erase(loot_objs_.begin() + idx);
    }

    const std::vector<std::shared_ptr<LootObj>>& GetLootObjs() const
    {
        return loot_objs_;
    }

    std::pair<double, double> GetRandomPointOnMap();

    size_t ItemsCount() const
    {
        return loot_objs_.size();
    }

    collision_detector::Item GetItem(size_t idx) const
    {
        collision_detector::Item ret = {{loot_objs_[idx]->x_,loot_objs_[idx]->y_}, loot_objs_[idx]->width_};
        return ret;
    }

    size_t GatherersCount() const
    {
        return dogs_.size();
    }

    collision_detector::Gatherer GetGatherer(size_t idx) const
    {
        collision_detector::Gatherer ret = {{dogs_[idx]->GetCurPos().first, dogs_[idx]->GetCurPos().second},
                                            {dogs_[idx]->GetPrevPos().first, dogs_[idx]->GetPrevPos().second},
                                            dogs_[idx]->GetWidth()};
        return ret;
    }

    int GetCountOfBases() const
    {
        return count_base_;
    }

private:
    int count_base_ = 0;
    std::vector<std::shared_ptr<Dog>> dogs_;
    std::vector<std::shared_ptr<LootObj>> loot_objs_;
    const Map* map_ptr_;
};

class Player {
public:
    using Id = util::Tagged<int, Dog>;

    explicit Player(GameSession* session, std::shared_ptr<Dog> player_dog, std::string& token_gen): session_(session), dog_(player_dog)
    {
        token = token_gen;
    }

    const Id& GetId() const noexcept {
        return dog_->GetId();
    }

    GameSession* GetSession() const {
        GameSession* ret = session_;
        return ret;
    }

    std::shared_ptr<Dog> GetDog() const {
        std::shared_ptr<Dog> doggy = dog_;
        return doggy;
    }

    const std::string& GetToken() const
    {
        return token;
    }

private:
std::string token;
GameSession* session_;
std::shared_ptr<Dog> dog_;

};

class Players
{
public:
    std::pair<std::string, std::shared_ptr<Player>> AddPlayer(GameSession* session ,std::shared_ptr<Dog> dog_ptr);
    std::pair<std::string, std::shared_ptr<Player>> AddPlayer(GameSession* session ,std::shared_ptr<Dog> dog_ptr, std::string token);
    std::shared_ptr<Player> FindByDogIdAndMapId(std::string dog_id, std::string map_id);
    std::optional<std::shared_ptr<Player>> FindByDogId(model::Dog::Id dog_id);
    std::optional<std::shared_ptr<Player>> FindByToken(std::string token);
private:
    std::vector<std::shared_ptr<Player>> players_;
    std::unordered_map<std::string, std::shared_ptr<Player>> players_map;


};

double model_random_generator();

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    void AddGameSession(Map::Id id);

    void RestoreGame(std::string& path);

    void BackupGame(std::string& path);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession* FindGameSession(const Map::Id& id) {
        if (auto it = game_session_id_to_index_.find(id); it != game_session_id_to_index_.end()) {
            return &sessions_.at(it->second);
        }
        return nullptr;
    }

    const std::vector<GameSession>& GetSessions() const
    {
        return sessions_;
    }

    void UpdateSessionsTime(std::chrono::milliseconds tick_ms);

    void AddRandomLoot(std::chrono::milliseconds period, double probability)
    {
        loot_g_ = std::make_shared<loot_gen::LootGenerator>(period, probability, model_random_generator);
    }

    unsigned GetNumberOfItemsToAdd(std::chrono::milliseconds time_delta, unsigned loot_count, unsigned looter_count) const;

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;


    using GameSessionIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    GameSessionIdToIndex game_session_id_to_index_;
    std::vector<GameSession> sessions_;
    std::shared_ptr<loot_gen::LootGenerator> loot_g_;
};

}  // namespace model
