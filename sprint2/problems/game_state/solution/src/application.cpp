#include "application.h"

#include <string>
#include <optional>
#include <iostream>

namespace json = boost::json;





namespace application {


std::optional<std::string> TryExtractToken(const StringRequest& request)
{
  	try{
       	std::string_view local_bearer_buf = request.base().at(http::field::authorization);
       	std::string received_token(local_bearer_buf.substr(7));
       	return received_token;
   	} catch(...)
   	{
       	return std::nullopt;
   	}
}

static std::string consider_dog_direction(int dir)
{
	std::string ret;
	if(dir == 0)
		ret.append("U");
	else if(dir == 1)
		ret.append("D");
	else if (dir == 2)
		ret.append("W");
	else if (dir == 3)
		ret.append("E");
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
    							req.method(), false, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
}


StringResponse ApplicationFacade::PlayersList(StringRequest& req)
{
	StringResponse ret;
	if(auto arg = TryExtractToken(req))
	{
		std::cout << *arg << std::endl;
		json::object obj;
		model::Player* buffered_player = players_.FindByToken(*arg);
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
    			local_obj["name"] = dogs[i].GetName();
        		obj[std::to_string(*dogs[i].GetId())] = local_obj;
    		}
    		std::string str;
    		str = json::serialize(obj);
    		std::string_view sv_str(str);
    		ret = MakeStringResponse(http::status::ok, std::move(sv_str), req.version(), req.keep_alive(),
    								 req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
	}
	else
	{
		ret = MakeStringResponse(http::status::unauthorized, R"({"code": "invalidToken", "message": "Auth not complete"})"sv,
        						 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
	}
	return ret;
	
}

StringResponse ApplicationFacade::PlayersState(StringRequest& req)
{
	StringResponse ret;
	if(auto arg = TryExtractToken(req))
	{
		std::cout << *arg << std::endl;
		
		model::Player* buffered_player = players_.FindByToken(*arg);
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
    			auto params = dogs[i].GetDogParams();
    			json::array arr_pos;
    			arr_pos.push_back(params.x);
    			arr_pos.push_back(params.y);
    			json::array arr_speed;
    			arr_speed.push_back(params.vx);
    			arr_speed.push_back(params.vy);
    			// json::value dog_dir(std::to_string(consider_dog_direction(params.direction)));
    			local_obj["pos"] = arr_pos;
    			local_obj["speed"] = arr_speed;
    			local_obj["dir"] = consider_dog_direction(params.direction);
        		obj[std::to_string(*dogs[i].GetId())] = local_obj;
    		}
    		json::value players_data{{"players"s,obj}};
    		std::string str;
    		str = json::serialize(players_data);
    		std::string_view sv_str(str);
    		ret = MakeStringResponse(http::status::ok, std::move(sv_str), req.version(), req.keep_alive(),
    								 req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
		}
	}
	else
	{
		ret = MakeStringResponse(http::status::unauthorized, R"({"code": "invalidToken", "message": "Auth not complete"})"sv,
        						 req.version(), req.keep_alive(), req.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
	}
	return ret;
}

}




 // application namespace


