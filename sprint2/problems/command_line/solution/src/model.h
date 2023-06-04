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

namespace model {

using Dimension = int;
using Coord = Dimension;

const std::map<std::string, int> dog_direction_map {{"NORTH", 0},
                                                   {"SOUTH", 1},
                                                   {"WEST", 2},
                                                   {"EAST", 3}
                                                   };

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
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
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
        return (((double)(start_.x <= end_.x)?start_.x:end_.x) - 0.4);
    }

    double RightBound() const noexcept{
        return (((double)(start_.x <= end_.x)?end_.x:start_.x) + 0.4);
    }

    double UpBound() const noexcept{
        return (((double)(start_.y <= end_.y)?start_.y:end_.y) - 0.4);
        
    }

    double LowBound() const noexcept{
        return (((double)(start_.y <= end_.y)?end_.y:start_.y) + 0.4);
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

    // Road* RoadPtrByIdx(int idx) const noexcept
    // {
    //     Road* ret = nullptr;
    //     if(idx >= 0 && idx < roads_.size())
    //         ret = (&roads_[idx]);
    //     return ret;
    // }

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

    void AddOffice(Office office);


private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    std::vector<int> horizontal_roads_idx;
    std::vector<int> vertical_roads_idxs;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double DogSpeed;
};



namespace detail {
struct TokenTag {};
}  // namespace detail

// using Token = util::Tagged<std::string, detail::TokenTag>;

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

class Dog{
public:
    using Id = util::Tagged<int, Dog>;
    Dog(std::string name, double x, double y, const Map* map_ptr, int idx): dog_name{std::move(name)}, id_(dog_count++), coords_{x, y}, 
                                                                map_ptr_(map_ptr), current_road_idx(idx) {}
                                                

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return dog_name;
    }

    const Dog_mv_param& GetDogParams() const noexcept {
        return coords_;
    }

    void SetDogOrientation(std::pair<int, int> basis, double speed, int dir)
    {
        coords_.vx = speed*basis.first;
        coords_.vy = speed*basis.second;
        coords_.direction = dir;
    }

    void Update(double tick_ms);

private:
    Dog_mv_param coords_;
    static int dog_count;
    size_t current_road_idx;
    const Map* map_ptr_;
    std::string dog_name;
    Id id_;
};



class GameSession
{
    using Id = util::Tagged<std::string, Map>;
    using Dogs = std::vector<Dog>;
public:
    GameSession(const Map* m_ptr):map_ptr_(m_ptr){}

    const Id& GetSessionMapId() const noexcept {
        return map_ptr_->GetId();
    }

    const std::vector<Dog*>& GetDogs() const noexcept {
        return dogs_;
    }

    const Map* GetMap()
    {
        return map_ptr_;
    }

    Dog* AddDog(std::string name, double x, double y, const Map* road_ptr, int road_idx);

    void UpdateDogs(double tick_ms);

private:
    std::vector<Dog*> dogs_;
    const Map* map_ptr_;
};

class Player {
public:
    using Id = util::Tagged<int, Dog>;

    explicit Player(GameSession* session, Dog* player_dog): session_(session), dog_(player_dog)
    {}

    std::string& get_player_name();

    const Id& GetId() const noexcept {
        return dog_->GetId();
    }

    GameSession* GetSession(){
        GameSession* ret = session_;
        return ret;
    }

    Dog* GetDog(){
        Dog* doggy = dog_;
        return doggy;
    }

private:
GameSession* session_;
Dog* dog_;

};

class Players
{
public:
    std::pair<std::string, Player*> AddPlayer(GameSession* session ,Dog* dog_ptr);
    Player* FindByDogIdAndMapId(std::string dog_id, std::string map_id);
    Player* FindByToken(std::string token);
private:
    std::vector<Player*> players_;
    std::unordered_map<std::string, Player*> players_map;


};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    void AddGameSession(Map::Id id);



    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession* FindGameSession(const Map::Id& id) noexcept {
        if (auto it = game_session_id_to_index_.find(id); it != game_session_id_to_index_.end()) {
            return &sessions_.at(it->second);
        }
        return nullptr;
    }

    void UpdateSessionsTime(double tick_ms);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;


    using GameSessionIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    GameSessionIdToIndex game_session_id_to_index_;
    std::vector<GameSession> sessions_;
};

}  // namespace model
