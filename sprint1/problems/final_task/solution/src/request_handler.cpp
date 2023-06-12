#include "request_handler.h"

#include "tagged.h"


#include <boost/json.hpp>
namespace json = boost::json;
namespace http_handler {

	constexpr static std::string_view MAP_STORAGE_BASED = "/api/v1/maps"sv;

	// Создаёт StringResponse с заданными параметрами
	StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, http::verb type,
                                  std::string_view content_type) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    if(type != http::verb::head)
        response.body() = body;
    if(status != http::status::ok)
        response.set(http::field::allow, "GET, HEAD"sv);
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

json::array GetRoadsFromMap(const model::Map* map_ptr)
{
    json::array roads;
    std::vector<model::Road> local_roads = map_ptr->GetRoads();
    std::vector<model::Road>::iterator it;
    for(it = local_roads.begin(); it != local_roads.end(); it++)
    {
        model::Point point_start = (*it).GetStart();
        model::Point point_end = (*it).GetEnd();
        json::object road_local;
        if((*it).IsHorizontal())
        {
            road_local["x0"] = point_start.x;
            road_local["y0"] = point_start.y;
            road_local["x1"] = point_end.x;
        }
        else
        {
            road_local["x0"] = point_start.x;
            road_local["y0"] = point_start.y;
            road_local["y1"] = point_end.y;
        }
        roads.push_back(road_local);
    }
    return roads;
}

json::array GetBuildingsFromMap(const model::Map* map_ptr)
{
    json::array buildings;
    std::vector<model::Building> local_building = map_ptr->GetBuildings();
    std::vector<model::Building>::iterator it;
    for(it = local_building.begin(); it != local_building.end(); it++)
    {
        model::Rectangle rect = (*it).GetBounds();
        json::object build_obj;
        build_obj["x"] = rect.position.x;
        build_obj["y"] = rect.position.y;
        build_obj["w"] = rect.size.width;
        build_obj["h"] = rect.size.height;
        buildings.push_back(build_obj);
    }
    return buildings;
}

json::array GetOfficesFromMap(const model::Map* map_ptr)
{
    json::array offices;
    std::vector<model::Office> local_office = map_ptr->GetOffices();
    std::vector<model::Office>::iterator it;
    for(it = local_office.begin(); it != local_office.end(); it++)
    {
        model::Office::Id id = (*it).GetId();
        model::Point pos = (*it).GetPosition();
        model::Offset offset = (*it).GetOffset();
        
        json::object office_obj;
        
        office_obj["id"] = *id;
        office_obj["x"] = pos.x;
        office_obj["y"] = pos.y;
        office_obj["offsetX"] = offset.dx;
        office_obj["offsetY"] = offset.dy;
        
        offices.push_back(office_obj);
    }
    return offices;
}

StringResponse RequestHandler::HandleRequest(StringRequest&& req, model::Game& game) {
    const auto text_response = [&req](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), req.method());
    };
    const auto json_response = [&req](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), req.method(), ContentType::APP_JSON);
    };
    enum class HandleMode{
        HANDLE_UNKNOWN_MODE,
        HANDLE_NEED_MAPS_LIST,
        HANDLE_NEED_MAP,
        HANDLE_NEED_API,
        HANDLE_ERR_MAP_NOT_FOUND,
        HANDLE_ERR_BAD_REQUEST
    };

    constexpr int API_PATH_LEN = 5;
    HandleMode mode = HandleMode::HANDLE_UNKNOWN_MODE;
    
    json::object obj;
    json::array arr;

    StringResponse ret;

    if(req.method() == http::verb::get || req.method() == http::verb::head)
    {
    	if(!req.target().compare(0, MAP_STORAGE_BASED.size(), MAP_STORAGE_BASED))
    	{
            if(MAP_STORAGE_BASED.size() == req.target().size())
            {
                mode = HandleMode::HANDLE_NEED_MAPS_LIST;

                auto maps = game.GetMaps();
            
                for(int i = 0; i < maps.size(); i++)
                {
                    json::object local_obj;
                    local_obj["id"] = *maps[i].GetId();
                    local_obj["name"] = maps[i].GetName();
                    arr.push_back(local_obj);
                }
            }
            else
            {
                std::string str_buf(req.target().substr(MAP_STORAGE_BASED.size() + 1));
                const model::Map* map_ptr = game.FindMap(util::Tagged<std::string, model::Map>(str_buf));
                if(map_ptr == nullptr)
                {
                    mode = HandleMode::HANDLE_ERR_MAP_NOT_FOUND;
                }
                else
                {
                    mode = HandleMode::HANDLE_NEED_MAP;
                    obj["id"] = *map_ptr->GetId();
                    obj["name"] = map_ptr->GetName();
                    obj["roads"] = GetRoadsFromMap(map_ptr);
                    obj["buildings"] = GetBuildingsFromMap(map_ptr);
                    obj["offices"] = GetOfficesFromMap(map_ptr);
                }

            }
    		
    	}
    	else if(!req.target().compare(0, API_PATH_LEN, MAP_STORAGE_BASED, 0, API_PATH_LEN))
    	{
            mode = HandleMode::HANDLE_ERR_BAD_REQUEST; // currently it's bad request
    	}
        else
            mode = HandleMode::HANDLE_UNKNOWN_MODE;

        std::string str;
        if(mode == HandleMode::HANDLE_NEED_MAPS_LIST)
            str = (json::serialize(arr));
        else
            str = (json::serialize(obj));

        std::string_view sv_str(str);
  

        if(mode == HandleMode::HANDLE_ERR_BAD_REQUEST || mode == HandleMode::HANDLE_UNKNOWN_MODE)
        {
            ret = json_response(http::status::bad_request, R"({"code": "badRequest", "message": "Bad Request"})"sv);/*"{\ncode: badRequest,\nmessage: BadRequest\n}"sv);*/
        }
        else if(mode == HandleMode::HANDLE_ERR_MAP_NOT_FOUND)
        {
            ret = json_response(http::status::not_found, R"({"code": "mapNotFound", "message": "Map not found"})"sv);
        }
        else
            ret = json_response(http::status::ok, sv_str); 
            
    }
    else
    {
        ret = text_response(http::status::method_not_allowed, "Invalid method"sv);
    }

    return ret;
    
}
 

}  // namespace http_handler
