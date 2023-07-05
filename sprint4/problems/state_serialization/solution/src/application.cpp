#include "application.h"

#include <string>
#include <optional>
#include <iostream>

#include <stdexcept>

#include "model.h"

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "model_serialization.h"

#include <fstream>

#include <filesystem>

namespace application {


namespace json = boost::json;

std::optional<std::string> TryExtractToken(const StringRequest& request)
{
  	try{
       	std::string_view local_bearer_buf = request.base().at(http::field::authorization);
       	std::string received_token(local_bearer_buf.substr(7));
       	if(received_token.size() < 32)
       		throw  std::length_error("token length");
       	return received_token;
   	} catch(...)
   	{
       	return std::nullopt;
   	}
}


bool TryExtractNotMatchContentType(const StringRequest& request)
{
	bool ret = true;
  	try{
       	std::string_view local_buf = request.base().at(http::field::content_type);
       	if(local_buf == "application/json")
       		ret = false;

   	} catch(...)
	{
		ret = true;
	}
	return ret;
}


static std::string consider_dog_direction(int dir)
{
	std::string ret;
	if(dir == model::DogDirection::DOG_MOVE_UP)
		ret.append("U");
	else if(dir == model::DogDirection::DOG_MOVE_DOWN)
		ret.append("D");
	else if (dir == model::DogDirection::DOG_MOVE_LEFT)
		ret.append("L");
	else if (dir == model::DogDirection::DOG_MOVE_RIGHT)
		ret.append("R");
	else if(dir == model::DogDirection::DOG_MOVE_STOP)
		ret = {""};
	return ret;
}




StringResponse ApplicationFacade::ListMap(const StringRequest req)
{
	auto maps = game_.GetMaps();

	json::array arr;
    
    for(int i = 0; i < maps.size(); i++)
    {
        json::object local_obj;
        local_obj["id"] = *maps[i].GetId();
        local_obj["name"] = maps[i].GetName();
        arr.push_back(local_obj);
    }

    std::string str;
    str = json::serialize(arr);
    std::string_view sv_str(str);

    return MakeStringResponse(http::status::ok, sv_str, req.version(), req.keep_alive(), 
    							req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
}


StringResponse ApplicationFacade::PlayersList(const StringRequest& req)
{
	return ExecuteAuthorized(req, [req, players = &players_](const std::string& token){
		StringResponse ret;

		json::object obj;
		std::optional<std::shared_ptr<model::Player>> buffered_player = players->FindByToken(token);
    	if(buffered_player == std::nullopt)
    	{
			ret = MakeStringResponse(http::status::unauthorized, R"({"code": "unknownToken", "message": "Player token has not been found"})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
    	else
    	{
    		auto session_ptr = (*buffered_player)->GetSession();
    		auto dogs = session_ptr->GetDogs();
    		for(int i = 0; i < dogs.size(); i++)
    		{   
    			json::object local_obj;
    			local_obj["name"] = dogs[i]->GetName();
        		obj[std::to_string(*dogs[i]->GetId())] = local_obj;
    		}
    		std::string str;
    		str = json::serialize(obj);
    		std::string_view sv_str(str);
    		ret = MakeStringResponse(http::status::ok, std::move(sv_str), req.version(), req.keep_alive(),
    								 req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
		return ret;
	});
}

StringResponse ApplicationFacade::PlayersState(const StringRequest& req)
{
	return ExecuteAuthorized(req, [req, players = &players_](const std::string& token){
    	
    	StringResponse ret;
        /* Выполняем действие B, используя токен, и возвращаем ответ */
        std::optional<std::shared_ptr<model::Player>> buffered_player = players->FindByToken(token);
    	if(buffered_player == std::nullopt)
    	{
			ret = MakeStringResponse(http::status::unauthorized, R"({"code": "unknownToken", "message": "Player token has not been found"})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
    	else
    	{
    		json::object obj;
    		json::object lost_objects;
    		auto session_ptr = (*buffered_player)->GetSession();
    		auto dogs = session_ptr->GetDogs();
    		auto lost_objs = session_ptr->GetLootObjs();

    		for(int i = 0; i < dogs.size(); i++)
    		{   
    			json::object local_obj;
    			auto params = dogs[i]->GetDogParams();
    			json::array arr_pos;
    			arr_pos.push_back(params.x);
    			arr_pos.push_back(params.y);
    			json::array arr_speed;
    			arr_speed.push_back(params.vx);
    			arr_speed.push_back(params.vy);
    			local_obj["pos"] = arr_pos;
    			local_obj["speed"] = arr_speed;
    			if(params.direction < 4)
    				local_obj["dir"] = consider_dog_direction(params.direction);
    			else
    				local_obj["dir"] = "";
    			auto loot_bag = dogs[i]->GetLootBag();
    			if(loot_bag.size() > 0)
    			{
    				json::array loot_bag_items;
    				for(auto bag_it = loot_bag.begin(); bag_it != loot_bag.end(); bag_it++)
    				{
    					loot_bag_items.push_back(json::object{{"id",(*bag_it)->id},{"type",(*bag_it)->type}});
    				}
    				local_obj["bag"] = loot_bag_items;
    			}
    			else
    				local_obj["bag"] = json::array();
    			local_obj["score"] = dogs[i]->GetScore();

        		obj[std::to_string(*dogs[i]->GetId())] = local_obj;
    		}

    		for(int i = 0; i < lost_objs.size(); i++)
    		{
    			json::object local_obj;
    			if(lost_objs[i]->type_ == model::LOOT_TYPE_BASE)
    				continue;
    			local_obj["type"] = lost_objs[i]->type_;
    			json::array arr_pos;
    			arr_pos.push_back(lost_objs[i]->x_);
    			arr_pos.push_back(lost_objs[i]->y_);
    			local_obj["pos"] = arr_pos;
    			lost_objects[std::to_string(i)] = local_obj;
    		}
    		json::object players_data;
    		players_data["players"] = obj;
    		players_data["lostObjects"] = lost_objects;
    		std::string str;
    		str = json::serialize(players_data);
    		std::string_view sv_str(str);
    		ret = MakeStringResponse(http::status::ok, std::move(sv_str), req.version(), req.keep_alive(),
    								 req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
		return ret;
    });
}


StringResponse ApplicationFacade::SetPlayerAction(const StringRequest& req) {
    
    return ExecuteAuthorized(req, [&req, players = &players_](const std::string& token){
    		StringResponse ret;
    		bool normal_mode = true;
    		std::optional<std::shared_ptr<model::Player>> buffered_player = players->FindByToken(token);
    		if(TryExtractNotMatchContentType(req))
    		{
    			ret = MakeStringResponse(http::status::bad_request, R"({"code": "invalidArgument", "message": "Invalid content-type"})"sv,
        						 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
    		}
    		else if(buffered_player == std::nullopt)
    		{
				ret = MakeStringResponse(http::status::unauthorized, R"({"code": "unknownToken", "message": "Player token has not been found"})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
			}
			else
			{
				try{	
    				std::string move = boost::json::value_to<std::string>(boost::json::parse(req.body()).at("move"));
					std::pair<int,int> dog_speed_basis;
					int dir = model::DogDirection::DOG_MOVE_UP;
					if(move == "L")
					{
						dir = model::DogDirection::DOG_MOVE_LEFT;
						dog_speed_basis = {-1,0};
					}
					else if(move == "R")
					{
						dir = model::DogDirection::DOG_MOVE_RIGHT;
						dog_speed_basis = {1,0};
					}
					else if(move == "U")
					{
						dir = model::DogDirection::DOG_MOVE_UP;
						dog_speed_basis = {0,-1};
					}
					else if(move == "D")
					{
						dir = model::DogDirection::DOG_MOVE_DOWN;
						dog_speed_basis = {0,1};
					}
					else if(move == "")
					{
						dir = model::DogDirection::DOG_MOVE_STOP;
						dog_speed_basis = {0,0};
					}
					else
						normal_mode = false;

					if(normal_mode)
					{
						auto session_ptr = (*buffered_player)->GetSession();
						auto map_ptr = session_ptr->GetMap();
						auto doggy = (*buffered_player)->GetDog();
						double speed = map_ptr->GetDogSpeed();
						doggy->SetDogOrientation(dog_speed_basis, speed, dir);
						ret = MakeStringResponse(http::status::ok, R"({})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
					}
				}
				catch(...)
				{
					normal_mode = false;
				}
			}

		if(!normal_mode)
		{
			ret = MakeStringResponse(http::status::unauthorized, R"({})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}

    	return ret;
    	});
}

	void ApplicationFacade::TryBackupTimer(int tick_ms)
	{
		if(backup_period == 0)
			return;
		prev_backup_period += tick_ms;
		if(prev_backup_period >= backup_period)
		{
			prev_backup_period = prev_backup_period - backup_period;
			BackupGame();
		}
	}


	void ApplicationFacade::TimerTickAuto(std::chrono::milliseconds tick_ms)
	{	
		game_.UpdateSessionsTime(tick_ms);
		TryBackupTimer(static_cast<int>(tick_ms.count()));
	}


	StringResponse ApplicationFacade::TimerTick(const StringRequest& req) {
	    StringResponse ret;

	    bool normal_mode = true;

		try{
				if(!((boost::json::parse(req.body()).at("timeDelta").is_number())))
	    			throw  std::invalid_argument("not number");
   				if(boost::json::parse(req.body()).at("timeDelta").is_double())
   					throw std::invalid_argument("not number");
   				int tick_time = boost::json::value_to<int>(boost::json::parse(req.body()).at("timeDelta"));
				std::chrono::milliseconds tick_ms(tick_time);
				game_.UpdateSessionsTime(tick_ms);
				TryBackupTimer(static_cast<int>(tick_ms.count()));


				if(normal_mode)
				{
					ret = MakeStringResponse(http::status::ok, R"({})"sv,
											 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
				}
			}
			catch(...)
			{
				normal_mode = false;
			}

		if(!normal_mode)
		{
			ret = MakeStringResponse(http::status::bad_request, R"({"code":"invalidArgument", "message": "Failed to parse tick request JSON"})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
	    return ret;
	}

	bool ApplicationFacade::GetTimerMode() const
	{
		return auto_timer_mode;
	}

	void ApplicationFacade::AutoTimerModeEnable()
	{
		auto_timer_mode = true;
	}

	void ApplicationFacade::SetRandomSpawn()
	{
		random_spawn_dogs = true;
	}

	bool ApplicationFacade::IsInRandomSpawnMode() const
	{
		return random_spawn_dogs;
	}


	std::optional<std::shared_ptr<app_support::loottypes_for_maps>> ApplicationFacade::GetExtraDataMap(std::string Id) const noexcept
	{
		return frontend_extra_data_.GetExtraData(std::move(Id));
	}

	void ApplicationFacade::InitBackupParams(std::string& path, int period)
	{
		backup_path = path;
		backup_period = period;
	}

	void ApplicationFacade::BackupGame()
	{
	    std::string temp_backup_path(backup_path);
	    temp_backup_path.append("_backup");
	    std::ofstream out_arch{temp_backup_path, std::ios_base::binary};
	    if(!out_arch.good())
	    	return;
	    boost::archive::binary_oarchive ar{out_arch};
	    auto local_sessions = game_.GetSessions();

	    serialization::GlobalArchieve ser_prepared;


	    if(local_sessions.size() > 0)
	    {
		    for(auto it = local_sessions.begin(); it < local_sessions.end(); it++)
		    {
		    	serialization::GameSessionRepr sessionRepr(*(*it));

		        auto dogs = (*it)->GetDogs();
		        for(int i = 0; i < dogs.size(); i++)
		        {
		            serialization::DogRepr dogSerBuf(*dogs[i]);
		            auto dog_id = dogs[i]->GetId();
		            auto token = (*players_.FindByDogId(dog_id))->GetToken();
		            sessionRepr.dog_reprs.push_back(dogSerBuf);
		            sessionRepr.tokens.push_back(token);
		        }
		        ser_prepared.game_session_reprs.push_back(sessionRepr);
		    }
		}

	    ar << ser_prepared;
	    try
	    {
	    	std::filesystem::rename(temp_backup_path, backup_path);
	    }
	    catch(...)
	    {
	    	return;
	    }
	}

	void ApplicationFacade::RestoreGame()
	{
	    std::ifstream in_arch{backup_path, std::ios_base::binary};

	    if(!in_arch.good())
	        return;

	    try
	    {
	        boost::archive::binary_iarchive ar{in_arch};
	        serialization::GlobalArchieve ser_prepared;
	        ar >> ser_prepared;
	        for(auto it = ser_prepared.game_session_reprs.begin(); it != ser_prepared.game_session_reprs.end(); it++)
	        {
	        	if((*it).dog_reprs.size() == 0)
	        	{
	        		continue;
	        	}
	        	model::Map::Id id((*it).map_id);
	        	game_.AddGameSession(id);
	        	auto session_ptr = game_.FindGameSession(id);
	        	auto current_map = game_.FindMap(id);
	        	for(size_t i = 0; i < (*it).dog_reprs.size(); i++)
	        	{
	        		auto restored_dog = (*it).dog_reprs[i].Restore(current_map);
	        		session_ptr->AddDog(restored_dog);
	        		players_.AddPlayer(session_ptr, restored_dog, (*it).tokens[i]);
	        	}

	        }
	    }
	    catch(...)
	    {
	        return;
	    }

	}

}




 // application namespace


