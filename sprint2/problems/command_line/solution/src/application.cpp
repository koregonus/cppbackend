#include "application.h"

#include <string>
#include <optional>
#include <iostream>

#include <stdexcept>

#include "model.h"

namespace json = boost::json;


namespace application {


std::optional<std::string> TryExtractToken(StringRequest& request)
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


bool TryExtractNotMatchContentType(StringRequest& request)
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




StringResponse ApplicationFacade::ListMap(StringRequest req)
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


StringResponse ApplicationFacade::PlayersList(StringRequest& req)
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
    		model::GameSession* session_ptr = (*buffered_player)->GetSession();
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

StringResponse ApplicationFacade::PlayersState(StringRequest& req)
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
    		model::GameSession* session_ptr = (*buffered_player)->GetSession();
    		auto dogs = session_ptr->GetDogs();

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

        		obj[std::to_string(*dogs[i]->GetId())] = local_obj;
    		}
    		json::value players_data{{"players"s,obj}};
    		std::string str;
    		str = json::serialize(players_data);
    		std::string_view sv_str(str);
    		ret = MakeStringResponse(http::status::ok, std::move(sv_str), req.version(), req.keep_alive(),
    								 req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
		return ret;
    });
}


StringResponse ApplicationFacade::SetPlayerAction(StringRequest& req) {
    
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
						model::GameSession* session_ptr = (*buffered_player)->GetSession();
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


	void ApplicationFacade::TimerTickAuto(std::chrono::milliseconds tick_ms)
	{
		double make_tick = (double)(tick_ms.count());
		game_.UpdateSessionsTime(make_tick);
	}


	StringResponse ApplicationFacade::TimerTick(StringRequest& req) {
	    StringResponse ret;

	    bool normal_mode = true;

		try{
				if(!((boost::json::parse(req.body()).at("timeDelta").is_number())))
	    			throw  std::invalid_argument("not number");
   				if(boost::json::parse(req.body()).at("timeDelta").is_double())
   					throw std::invalid_argument("not number");
   				int tick_time = boost::json::value_to<int>(boost::json::parse(req.body()).at("timeDelta"));
				game_.UpdateSessionsTime((double)tick_time);

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

	bool ApplicationFacade::GetTimerMode()
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

	bool ApplicationFacade::IsInRandomSpawnMode()
	{
		return random_spawn_dogs;
	}



}




 // application namespace


