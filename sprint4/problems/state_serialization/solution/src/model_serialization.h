#include <boost/serialization/vector.hpp>

#include "model.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, model::DoggyLoot& obj, [[maybe_unused]] const unsigned version) {
    ar&(obj.id);
    ar&(obj.type);
}

template <typename Archive>
void serialize(Archive& ar, model::LootObj& obj, [[maybe_unused]] const unsigned version) {
    ar&(obj.type_);
    ar&(obj.x_);
    ar&(obj.y_);
    ar&(obj.width_);
    ar&(obj.value_);
}

}  // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPositionPoint())
        , bag_capacity_(dog.GetLootBagSize())
        , speed_(dog.GetDogSpeedVec())
        , direction_(dog.GetDogDirection())
        , score_(dog.GetScore())
        , road_idx(dog.GetRoadIdx()) {
        auto loot_bag = dog.GetLootBag();
        for(const auto& item : loot_bag)
        {
            bag_content_.push_back(model::DoggyLoot(item->id, item->type));
        }
    }

    [[nodiscard]] std::shared_ptr<model::Dog> Restore(const model::Map* map_ptr) const {
        std::shared_ptr<model::Dog> dog_ptr = std::make_shared<model::Dog>(id_, name_, pos_.x, pos_.y, map_ptr, road_idx);
        dog_ptr->TrySetDogCount(*id_);
        dog_ptr->SetDogSpeedVec(speed_);
        dog_ptr->SetDogDirection(direction_);
        dog_ptr->AddToScore(score_);
        for (const auto& item : bag_content_) 
        {
            dog_ptr->AddToLootBag(item.id, item.type);
        }
        return dog_ptr;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& name_;
        ar& pos_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    geom::Point2D pos_;
    size_t bag_capacity_ = 0;
    geom::Vec2D speed_;
    int direction_ = model::DogDirection::DOG_MOVE_UP;
    int score_ = 0;
    std::vector<model::DoggyLoot> bag_content_;
    int road_idx = 0;
};



struct GameSessionRepr
{
    GameSessionRepr() = default;

    GameSessionRepr(model::GameSession& session)
    {
        auto map = session.GetMap();
        map_id = (*map->GetId());
        auto loot = session.GetLootObjs();
        for(int i = 0; i < loot.size(); i++)
        {
            if(i < session.GetCountOfBases())
            {
                continue;
            }
            session_loot.push_back(model::LootObj(loot[i]->type_,loot[i]->x_,loot[i]->y_,loot[i]->width_, loot[i]->value_));
        }

    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& map_id;
        ar& dog_reprs;
        ar& tokens;
        ar& session_loot;
    }

    std::string map_id;
    std::vector<DogRepr> dog_reprs;
    std::vector<std::string> tokens;
    std::vector<model::LootObj> session_loot;
};

struct GlobalArchieve
{
    std::vector<GameSessionRepr> game_session_reprs;
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {

        ar& game_session_reprs;
    }
};

/* Другие классы модели сериализуются и десериализуются похожим образом */

}  // namespace serialization
