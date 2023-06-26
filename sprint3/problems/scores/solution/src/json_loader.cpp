#include "json_loader.h"


#include <fstream>
#include <iostream>
#include <vector>


#include "tagged.h"

#include <chrono>

#include "application_support.h"

namespace json_loader {

    namespace json = boost::json;

    using namespace std::chrono_literals;

    static std::string json_as_string(const std::filesystem::path& json_path)
    {
        std::ifstream ifs(json_path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
        std::ifstream::pos_type fileSize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        std::vector<char> bytes(fileSize);
        ifs.read(bytes.data(), fileSize);

        return std::string(bytes.data(), fileSize);
    }

    static void add_roads_to_game(const json::value& map, model::Map& local_map) {
        if(map.as_object().if_contains("roads"))
        {
            auto roads_array = map.at("roads").as_array();
            for(int i = 0; i < roads_array.capacity(); i++)
            {
                auto road = roads_array.at(i).as_object();
                if(road.if_contains("x1"))
                {   
                    // horizontal
                    int x0 = static_cast<int>(road.at("x0").get_int64());
                    int y0 = static_cast<int>(road.at("y0").get_int64());
                    int x1 = static_cast<int>(road.at("x1").get_int64());
                    model::Point start_point = {x0, y0};
                    auto newroad = model::Road(model::Road::HORIZONTAL, start_point, x1);
                    local_map.AddRoad(newroad);
                }
                else if(road.if_contains("y1"))
                {
                    // vertical
                    model::Point start_point = {static_cast<int>(road.at("x0").get_int64()), static_cast<int>(road.at("y0").get_int64())};
                    auto newroad = model::Road(model::Road::VERTICAL, start_point, static_cast<int>(road.at("y1").get_int64()));
                    local_map.AddRoad(newroad);
                }
                else
                {
                    std::cout << "bad road";
                }
            }
        }
    }

    static void add_buildings_to_game(const json::value& map, model::Map& local_map) {
        if(map.as_object().if_contains("buildings"))
        {
            auto buildings_array = map.at("buildings").as_array();
            for(int i = 0; i < buildings_array.capacity(); i++)
            {
                auto build = buildings_array.at(i).as_object();
                try
                {
                    model::Point start_point = {static_cast<int>(build.at("x").get_int64()), static_cast<int>(build.at("y").get_int64())};
                    model::Size build_size = {static_cast<int>(build.at("w").get_int64()), static_cast<int>(build.at("h").get_int64())};
                    local_map.AddBuilding(model::Building{model::Rectangle{start_point, build_size}});
                }
                catch(...)
                {
                    continue;
                }
            }
        }
    }

    static void add_offices_to_game(const json::value& map, model::Map& local_map) {
        if(map.as_object().if_contains("offices"))
        {
            auto offices_array = map.at("offices").as_array();
            for(int i = 0; i < offices_array.capacity(); i++)
            {
                auto office = offices_array.at(i).as_object();
                try
                {
                    auto office_id = std::make_shared<std::string>(std::move(office.at("id").as_string()));
                    model::Office::Id id(*office_id);

                    model::Point start_point = {static_cast<int>(office.at("x").get_int64()), static_cast<int>(office.at("y").get_int64())};
                    model::Offset office_offset = {static_cast<int>(office.at("offsetX").get_int64()), static_cast<int>(office.at("offsetY").get_int64())};
                    local_map.AddOffice(model::Office{id, start_point, office_offset});
                }
                catch(...)
                {
                    continue;
                }
            }
        }
    }



    static void parse_map(const json::value& map, model::Game& game, double defaultDogSpeed, 
                          int64_t defaultBagCapacity,  app_support::FrontendExtraDataMap& extra_data)
    {
        if(!(map.as_object().if_contains("id") && map.as_object().if_contains("name")))
            return;
        auto map_id = std::make_shared<std::string>(std::move(map.at("id").as_string()));
        auto map_name = std::make_shared<std::string>(std::move(map.at("name").as_string()));
        model::Map::Id id(*map_id);

        model::Map local_map(id, *map_name);

        add_roads_to_game(map, local_map);
        add_buildings_to_game(map, local_map);
        add_offices_to_game(map, local_map);

        // Добавляем скорость собаки для мапы в случае если она указана
        if(map.as_object().if_contains("dogSpeed"))
            defaultDogSpeed = map.at("dogSpeed").as_double();
        local_map.SetDogSpeed(defaultDogSpeed);
        std::shared_ptr local_extra_map = std::make_shared<app_support::loottypes_for_maps>();
        if(map.as_object().if_contains("lootTypes"))
        {
            auto loot_array = map.as_object().at("lootTypes").as_array();
            local_extra_map->count = loot_array.capacity();
            local_extra_map->lootTypes = std::make_shared<json::array>(loot_array);
            for(int i = 0; i < loot_array.capacity(); i++)
            {   
                auto loot_element = loot_array.at(i).as_object();
                if(loot_element.if_contains("value"))
                {
                    int elm_value = static_cast<int>(loot_element.at("value").as_int64());
                    local_map.AddLootValue(i, elm_value);
                }
            }
        }
        local_map.SetLootObjsCount(static_cast<unsigned>(map.as_object().at("lootTypes").as_array().capacity()));
        if(map.as_object().if_contains("bagCapacity"))
            defaultBagCapacity = map.at("bagCapacity").as_uint64();
        local_map.SetBagCapacity(defaultBagCapacity);
        game.AddMap(local_map);
        extra_data.AddExtraData(*map_id, local_extra_map);
    }

    model::Game LoadGame(const std::filesystem::path& json_path, app_support::FrontendExtraDataMap& extra_data) {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя boost::json::parse
        // Загрузить модель игры из файла
        std::string json_from_file = json_as_string(json_path);

        auto value = json::parse(json_from_file);

        auto mp = value.as_object().at("maps");

        double defaultDogSpeed = 1.0;

        double period = 1.0;

        double probability = 0.5;

        uint64_t defaultBagCapacity = 3;

        if(value.as_object().if_contains("defaultDogSpeed"))
        {
            defaultDogSpeed = value.at("defaultDogSpeed").as_double();
        }
        if(value.as_object().if_contains("defaultBagCapacity"))
        {
            defaultBagCapacity = value.at("defaultBagCapacity").as_uint64();
        }

        if(value.as_object().if_contains("lootGeneratorConfig"))
        {
            if(value.as_object().at("lootGeneratorConfig").as_object().if_contains("period"))
                period = value.as_object().at("lootGeneratorConfig").as_object().at("period").as_double();
            if(value.as_object().at("lootGeneratorConfig").as_object().if_contains("probability"))
                probability = value.as_object().at("lootGeneratorConfig").as_object().at("probability").as_double();
        }

        model::Game game;

        auto period_cast = std::chrono::duration_cast<std::chrono::milliseconds>(period * 1000ms);
        game.AddRandomLoot(period_cast, probability);
    
        for(int i = 0; i < mp.as_array().capacity(); i++)
        {
            const json::value& ref_map = mp.as_array().at(i).as_object();
            model::Game& ref_game = game;
            parse_map(ref_map, ref_game, defaultDogSpeed, defaultBagCapacity, extra_data);
        }
        return game;
    }

}  // namespace json_loader
