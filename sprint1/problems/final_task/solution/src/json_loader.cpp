#include "json_loader.h"


#include <fstream>
#include <iostream>
#include <vector>

#include <boost/json.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "tagged.h"

namespace json_loader {

// using namespace std::literals;

namespace json = boost::json;


std::string json_as_string(const std::filesystem::path& json_path)
{
    std::ifstream ifs(json_path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    std::ifstream::pos_type fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<char> bytes(fileSize);
    ifs.read(bytes.data(), fileSize);

    return std::string(bytes.data(), fileSize);
}


void parse_map(const json::value& map, model::Game& game)
{
    try
    {
        if(!(map.as_object().if_contains("id") && map.as_object().if_contains("name")))
            return;
        auto map_id = std::make_shared<std::string>(std::move(map.at("id").as_string()));
        auto map_name = std::make_shared<std::string>(std::move(map.at("name").as_string()));
        model::Map::Id id(*map_id);

        model::Map local_map(id, *map_name);

        if(map.as_object().if_contains("roads"))
        {
            auto roads_array = map.at("roads").as_array();
            for(int i = 0; i < roads_array.capacity(); i++)
            {
                auto road = map.at("roads").as_array().at(i).as_object();
                if(road.if_contains("x1"))
                {   
                    // horizontal
                    model::Point start_point = {(int)road.at("x0").get_int64(), (int)road.at("y0").get_int64()};
                    auto newroad = model::Road(model::Road::HORIZONTAL, start_point, (int)road.at("x1").get_int64());
                    local_map.AddRoad(newroad);
                }
                else if(road.if_contains("y1"))
                {
                    // vertical
                    model::Point start_point = {(int)road.at("x0").get_int64(), (int)road.at("y0").get_int64()};
                    auto newroad = model::Road(model::Road::VERTICAL, start_point, (int)road.at("y1").get_int64());
                    local_map.AddRoad(newroad);
                }
                else
                {
                    std::cout << "bad road";
                }
            }
        }
        if(map.as_object().if_contains("buildings"))
        {
            auto buildings_array = map.at("buildings").as_array();
            for(int i = 0; i < buildings_array.capacity(); i++)
            {
                auto build = map.at("buildings").as_array().at(i).as_object();
                try
                {
                    model::Point start_point = {(int)build.at("x").get_int64(), (int)build.at("y").get_int64()};
                    model::Size build_size = {(int)build.at("w").get_int64(), (int)build.at("h").get_int64()};
                    local_map.AddBuilding(model::Building{model::Rectangle{start_point, build_size}});
                }
                catch(...)
                {
                    continue;
                }
            }
        }
        if(map.as_object().if_contains("offices"))
        {
            auto offices_array = map.at("offices").as_array();
            for(int i = 0; i < offices_array.capacity(); i++)
            {
                auto office = map.at("offices").as_array().at(i).as_object();
                try
                {
                    auto office_id = std::make_shared<std::string>(std::move(office.at("id").as_string()));
                    model::Office::Id id(*office_id);

                    model::Point start_point = {(int)office.at("x").get_int64(), (int)office.at("y").get_int64()};
                    model::Offset office_offset = {(int)office.at("offsetX").get_int64(), (int)office.at("offsetY").get_int64()};
                    local_map.AddOffice(model::Office{id, start_point, office_offset});
                }
                catch(...)
                {
                    continue;
                }
            }
        }
        
        game.AddMap(local_map);

    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    std::string json_from_file = json_as_string(json_path);

    auto value = json::parse(json_from_file);

    auto mp = value.as_object().at("maps");

    model::Game game;
    
    for(int i = 0; i < mp.as_array().capacity(); i++)
    {
        const json::value& ref_map = mp.as_array().at(i).as_object();
        model::Game& ref_game = game;
        parse_map(ref_map, ref_game);
    }
    return game;
}

}  // namespace json_loader
