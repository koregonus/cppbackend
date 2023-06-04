#include "application.h"

#include <string>
#include <optional>
#include <iostream>

#include <stdexcept>

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
       	// std::string received_content_type(local_buf.substr(7));
       	// if(received_token.size() < 32)
       	// 	throw  std::length_error("token length");
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
	if(dir == 0)
		ret.append("U");
	else if(dir == 1)
		ret.append("D");
	else if (dir == 2)
		ret.append("L");
	else if (dir == 3)
		ret.append("R");
	else if(dir == 4)
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
		model::Player* buffered_player = players->FindByToken(token);
    	if(buffered_player == nullptr)
    	{
			ret = MakeStringResponse(http::status::unauthorized, R"({"code": "unknownToken", "message": "Player token has not been found"})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
    	else
    	{
    		model::GameSession* session_ptr = buffered_player->GetSession();
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
        model::Player* buffered_player = players->FindByToken(token);
		if(buffered_player == nullptr)
    	{
			ret = MakeStringResponse(http::status::unauthorized, R"({"code": "unknownToken", "message": "Player token has not been found"})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
    	else
    	{
    		json::object obj;
    		model::GameSession* session_ptr = buffered_player->GetSession();
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
    		model::Player* buffered_player = players->FindByToken(token);
    		if(TryExtractNotMatchContentType(req))
    		{
    			ret = MakeStringResponse(http::status::bad_request, R"({"code": "invalidArgument", "message": "Invalid content-type"})"sv,
        						 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
    		}
    		else if(buffered_player == nullptr)
    		{
				ret = MakeStringResponse(http::status::unauthorized, R"({"code": "unknownToken", "message": "Player token has not been found"})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
			}
			else
			{
				try{	
    				std::string move = boost::json::value_to<std::string>(boost::json::parse(req.body()).at("move"));
					std::pair<int,int> dog_speed_basis;
					int dir = 0;
					if(move == "L")
					{
						dir = 2;
						dog_speed_basis = {-1,0};
					}
					else if(move == "R")
					{
						dir = 3;
						dog_speed_basis = {1,0};
					}
					else if(move == "U")
					{
						dir = 0;
						dog_speed_basis = {0,-1};
					}
					else if(move == "D")
					{
						dir = 1;
						dog_speed_basis = {0,1};
					}
					else if(move == "")
					{
						dir = 4;
						dog_speed_basis = {0,0};
						// std::cout << "move nothing\n";
					}
					else
						normal_mode = false;

					if(normal_mode)
					{
						model::GameSession* session_ptr = buffered_player->GetSession();
						auto map_ptr = session_ptr->GetMap();
						auto doggy = buffered_player->GetDog();
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
    	// } catch(...)
    	// {

    	// }
    	return ret;
    	});
}


StringResponse ApplicationFacade::TimerTick(StringRequest& req) {
    // std::cout << "TICK:: \n" << std::endl;
    // return ExecuteAuthorized(req, [&req, players = &players_](const std::string& token){
    		StringResponse ret;

    		bool normal_mode = true;
    	// 	model::Player* buffered_player = players->FindByToken(token);
    	// 	if(TryExtractNotMatchContentType(req))
    	// 	{
    	// 		ret = MakeStringResponse(http::status::bad_request, R"({"code": "invalidArgument", "message": "Invalid content-type"})"sv,
        // 						 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
    	// 	}
    	// 	else if(buffered_player == nullptr)
    	// 	{
		// 		ret = MakeStringResponse(http::status::unauthorized, R"({"code": "unknownToken", "message": "Player token has not been found"})"sv,
		// 						 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		// 	}
		// 	else
		// 	{
				try{
					if(!((boost::json::parse(req.body()).at("timeDelta").is_number())))
    					throw  std::invalid_argument("not number");

    				if(boost::json::parse(req.body()).at("timeDelta").is_double())
    					throw std::invalid_argument("not number");

    				// double tick_time_d = 0.0;
    				int tick_time = boost::json::value_to<int>(boost::json::parse(req.body()).at("timeDelta"));
    				
					// int tick_time = std::stoi(tick);
					// std::cout << "tick" << tick_time << std::endl;
					game_.UpdateSessionsTime((double)tick_time);
    				// std::pair<int,int> dog_speed_basis;
		// 			if(move == "L")
		// 				dog_speed_basis = {-1,0};
		// 			else if(move == "R")
		// 				dog_speed_basis = {1,0};
		// 			else if(move == "U")
		// 				dog_speed_basis = {0,1};
		// 			else if(move == "D")
		// 				dog_speed_basis = {0,-1};
		// 			else if(move == "")
		// 				dog_speed_basis = {0,0};
		// 			else
		// 				normal_mode = false;

					if(normal_mode)
					{
		// 				model::GameSession* session_ptr = buffered_player->GetSession();
		// 				auto map_ptr = session_ptr->GetMap();
		// 				auto doggy = buffered_player->GetDog();
		// 				double speed = map_ptr->GetDogSpeed();
		// 				doggy->SetDogOrientation(dog_speed_basis, speed);
						ret = MakeStringResponse(http::status::ok, R"({})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
					}
				}
				catch(...)
				{
					normal_mode = false;
				}
			// }

		if(!normal_mode)
		{
			ret = MakeStringResponse(http::status::bad_request, R"({"code":"invalidArgument", "message": "Failed to parse tick request JSON"})"sv,
								 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
    	return ret;
    	// });
}

}




 // application namespace


