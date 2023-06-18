#include <cmath>
#include <catch2/catch_test_macros.hpp>

#include "../src/model.h"
#include "../src/json_loader.h"

using namespace std::literals;

SCENARIO("Model testing") {
        GIVEN("a map with 1 road") {
        	model::Game game;
        	model::Map::Id id(std::string("map1"));
        	model::Map local_map(id, "MAP");
        	local_map.SetLootObjsCount(1);
        	model::Point start_point = {0, 0};
            auto newroad = model::Road(model::Road::VERTICAL, start_point, 10);
            local_map.AddRoad(newroad);
            game.AddMap(local_map);
            game.AddGameSession(id);
            game.AddRandomLoot(100ms, 0.1);
            auto session = game.FindGameSession(id);
            session->AddDog("snoop", 0.0, 0.0, &local_map, 0);
            WHEN("time not moving") {
	            THEN("loot will not be generated") {
	            	auto loot = session->GetLootObjs();
	                REQUIRE(loot.size() == 0);
	            }
	        }
    }
    	GIVEN("a map with 1 road") {
        	model::Game game;
        	model::Map::Id id(std::string("map1"));
        	model::Map local_map(id, "MAP");
        	local_map.SetLootObjsCount(1);
        	model::Point start_point = {0, 0};
            auto newroad = model::Road(model::Road::VERTICAL, start_point, 10);
            local_map.AddRoad(newroad);
            game.AddMap(local_map);
            game.AddGameSession(id);
            game.AddRandomLoot(100ms, 0.1);
            auto session = game.FindGameSession(id);
            session->AddDog("snoop", 0.0, 0.0, &local_map, 0);

            WHEN("one player connected") {
	            THEN("loot will be generated in required period once") {
	                for (unsigned looters = 0; looters < 10; ++looters) {
	                	game.UpdateSessionsTime(100ms);
	                }
	                auto loot = session->GetLootObjs();
	                REQUIRE(loot.size() == 1);
	            }
        	}
    }
}