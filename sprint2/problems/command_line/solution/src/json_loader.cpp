#include "json_loader.h"


#include <fstream>
#include <iostream>
#include <vector>

#include <boost/json.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "tagged.h"

namespace json_loader {

    namespace json = boost::json;

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



    static void parse_map(const json::value& map, model::Game& game, double defaultDogSpeed)
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
        game.AddMap(local_map);
    }

    model::Game LoadGame(const std::filesystem::path& json_path) {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя boost::json::parse
        // Загрузить модель игры из файла
        std::string json_from_file = json_as_string(json_path);

        auto value = json::parse(json_from_file);

        auto mp = value.as_object().at("maps");

        double defaultDogSpeed = 1.0f;

        if(value.as_object().if_contains("defaultDogSpeed"))
        {
            defaultDogSpeed = value.at("defaultDogSpeed").as_double();
        }

        model::Game game;
    
        for(int i = 0; i < mp.as_array().capacity(); i++)
        {
            const json::value& ref_map = mp.as_array().at(i).as_object();
            model::Game& ref_game = game;
            parse_map(ref_map, ref_game, defaultDogSpeed);
        }
        return game;
    }

}  // namespace json_loader
