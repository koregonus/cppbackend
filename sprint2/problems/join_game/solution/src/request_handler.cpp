#include "request_handler.h"

#include "tagged.h"

#include <map>
// #include <string>

#include <boost/json.hpp>
namespace json = boost::json;


namespace http_handler {

    // the variant to visit
    // using var_t = std::variant<http::string_body, http::file_body>;

    const std::map<std::string, std::string> static_parse_map {{".htm", "text/html"},
                                                        {".html", "text/html"},
                                                        {".css", "text/css"},
                                                        {".js", "text/javascript"},
                                                        {".json", "application/json"},
                                                        {".xml", "application/xml"},
                                                        {".png", "image/png"},
                                                        {".jpg", "image/jpeg"},
                                                        {".jpe", "image/jpeg"},
                                                        {".jpeg", "image/jpeg"},
                                                        {".gif", "image/gif"},
                                                        {".bmp", "image/bmp"},
                                                        {".ico", "image/vnd.microsoft.icon"},
                                                        {".tiff", "image/tiff"},
                                                        {".tif", "image/tiff"},
                                                        {".svg", "image/svg+xml"},
                                                        {".svgz", "image/svg+xml"},
                                                        {".mp3", "audio/mp3"}
                                                        };

	

    char decode_single_char(char to_decode)
    {
        char ret = 0;
        if(to_decode < 0x3AU)
        {
            ret = to_decode - '0';
        }
        else if(to_decode > 0x41U && to_decode < 0x47U)
        {
            ret = to_decode - 0x40U + 0xAU;
        }
        else if(to_decode > 0x60U && to_decode < 0x67U)
        {
            ret = to_decode - 0x61U + 0xAU;
        }


        // std::cout << ret << std::endl;

        return ret;
    }

    std::string decode_uri(const std::basic_string_view<char> str_from_target)
    {
        std::string ret;
        for(int i = 0; i < str_from_target.length(); i++)
        {
            if(str_from_target[i] == 37)
            {
                char buffer = ((decode_single_char(str_from_target[i+1])) * 16) + decode_single_char(str_from_target[i+2]);
                ret.push_back(buffer);
                i += 2;
            }
            else if(str_from_target[i] == '+')
            {
                ret.push_back(' ');
            }
            else
            {
                ret.push_back(str_from_target[i]);
            }
        }
        // std::cout << ret << std::endl;
        return ret;
    }


    // Возвращает true, если каталог p содержится внутри base_path.
    bool IsSubPath(fs::path path, fs::path base) {
        // Приводим оба пути к каноничному виду (без . и ..)
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }


	// Создаёт StringResponse с заданными параметрами
	StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, http::verb type, bool cache_control,
                                  std::string_view content_type, std::string_view allowed_method) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        if(type != http::verb::head)
            response.body() = body;
        if(status == http::status::method_not_allowed)
            response.set(http::field::allow, allowed_method);
        if(cache_control)
            response.set(http::field::cache_control, "no-cache");
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

// Создаёт StringResponse с заданными параметрами
    FileResponse MakeFileResponse(http::status status, std::string_view filepath, unsigned http_version,
                                  bool keep_alive, http::verb type,
                                  std::string_view content_type) {

        FileResponse response(status, http_version);
        response.set(http::field::content_type, content_type);

        http::file_body::value_type file;
        if(sys::error_code ec; file.open(std::string(filepath).data(), beast::file_mode::read, ec), ec)
        {
            std::cout << "Failed to open file"sv << filepath << std::endl;
        }
        else
        {
            // if(type != http::verb::head)
                response.body() = std::move(file);
            response.prepare_payload();
        }

        return response;
    }

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeFileHeadResponse(http::status status, std::string_view filepath, unsigned http_version,
                                  bool keep_alive, http::verb type,
                                  std::string_view content_type) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        // if(type != http::verb::head)
            // response.body() = body;
        if(status != http::status::ok)
            response.set(http::field::allow, "GET, HEAD"sv);
        try {
            // std::cout << "Attempt to get size of a directory:\n";
            response.content_length(fs::file_size(std::string(filepath)));
            ;
        } catch(fs::filesystem_error& e) {
            std::cout << e.what() << '\n';
        }
        // response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    std::string get_content_type_by_filetype(std::string type)
    {
        std::string ret;
        try
        {
            ret = static_parse_map.at((type));
        }
        catch(...)
        {
            ret = std::string("application/octet-stream");
        }

        return ret;
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


    var_t RequestHandler::HandleRequest(StringRequest&& req, model::Game& game, model::Players& players, fs::path& stat_folder, std::shared_ptr<struct SupportResponseLogger> support_log) {

        const auto text_response = [&req](http::status status, std::string_view text) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), req.method(), false);
        };
        const auto json_response = [&req](http::status status, std::string_view text) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), req.method(), false, ContentType::APP_JSON);
        };
        const auto file_response = [&req](http::status status, std::string_view text, std::string_view ContentType) {
            return MakeFileResponse(status, text, req.version(), req.keep_alive(), req.method(), ContentType);
        };
        const auto file_head_response =  [&req](http::status status, std::string_view text, std::string_view ContentType){
            return MakeFileHeadResponse(status, text, req.version(), req.keep_alive(), req.method(), ContentType);
        };
        

        
        HandleMode mode = HandleMode::HANDLE_UNKNOWN_MODE;
    
        json::object obj;
        json::array arr;

        var_t ret;

        std::string decoded_req_target = decode_uri(req.target());
        std::string generated_path(stat_folder);
        generated_path.append(decoded_req_target);

        // std::cout << generated_path << std::endl;
        // std::cout << stat_folder << std::endl;

        if(std::filesystem::equivalent(fs::weakly_canonical(generated_path), fs::weakly_canonical(stat_folder)))
        {
            // std::cout << "equal!!!\n";
            // generated_path = stat_folder;
            generated_path.append("/index.html");
        }

        

        // std::cout << generated_path << std::endl;

        // if(IsSubPath(fs::weakly_canonical(generated_path), fs::weakly_canonical(stat_folder)))
        // {
        //     std::cout << "we're in stat mode" << std::endl;
        // }

        // std::cout << fs::weakly_canonical(stat_folder) << std::endl;
        // std::cout << stat_folder.concat(decoded_req_target) << std::endl;
        // std::cout << fs::weakly_canonical(generated_path) << std::endl;

        if(req.method() != http::verb::get && req.method() != http::verb::head)
        {
            ret = text_response(http::status::method_not_allowed, "Invalid method"sv);
        }
        else
        {
           if(IsSubPath(fs::weakly_canonical(generated_path), fs::weakly_canonical(stat_folder)))
           {
                // std::cout << "we're in stat mode" << std::endl;
                if(!fs::exists(generated_path))
                {
                    // std::cout << "it's not exist!\n";
                    // std::cout << generated_path << std::endl;
                    mode = HandleMode::HANDLE_ERR_PAGE_NOT_FOUND;
                }
                else
                {
                    // std::cout << "it's exist!\n";
                    mode = HandleMode::HANDLE_NEED_STATIC_CONTENT;
                }
           }
           else
                mode = HandleMode::HANDLE_UNKNOWN_MODE;

            if(mode == HandleMode::HANDLE_NEED_STATIC_CONTENT)
            {
                if(req.method() == http::verb::get)
                    ret = file_response(http::status::ok, generated_path, get_content_type_by_filetype(std::move(fs::path(generated_path).extension())));
                else
                    ret = file_head_response(http::status::ok, generated_path, get_content_type_by_filetype(std::move(fs::path(generated_path).extension())));

            }
            else/*(mode == HandleMode::HANDLE_ERR_PAGE_NOT_FOUND)*/
            {
                ret = text_response(http::status::not_found, R"(Page not found)"sv);
            }
        }
        // return std::get<http::response<http::string_body>>(ret);

        // auto sup_ptr = support_log.get();
        std::visit([sup_ptr = support_log.get()](auto&& args){
            sup_ptr->result = args.result();
            sup_ptr->result_int = args.result_int();
            sup_ptr->content_type = args.base().at(http::field::content_type);
        }, ret);
        // sup_ptr->result_int = ret.result_int();
        return ret;
        // return ret;
    }

    var_t RequestHandler::HandleApiRequest(StringRequest req, model::Game& game, model::Players& players, fs::path& stat_folder, std::shared_ptr<struct SupportResponseLogger> support_log)
    {
        const auto json_response = [&req](http::status status, std::string_view text, bool cache_control, std::string_view allowed_method) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), req.method(), cache_control, ContentType::APP_JSON, allowed_method);
        };
        const auto file_response = [&req](http::status status, std::string_view text, std::string_view ContentType) {
            return MakeFileResponse(status, text, req.version(), req.keep_alive(), req.method(), ContentType);
        };
        const auto file_head_response =  [&req](http::status status, std::string_view text, std::string_view ContentType){
            return MakeFileHeadResponse(status, text, req.version(), req.keep_alive(), req.method(), ContentType);
        };


        HandleMode mode = HandleMode::HANDLE_UNKNOWN_MODE;
    
        json::object obj;
        json::array arr;


        std::string decoded_req_target = decode_uri(req.target());
        std::string generated_path(stat_folder);
        generated_path.append(decoded_req_target);

        // std::cout << "ENTERED GRAY ZONE\n";
        // std::cout << decoded_req_target << std::endl;

        var_t ret;

        if(!decoded_req_target.compare(0, MAP_STORAGE_BASED.size(), MAP_STORAGE_BASED))
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
                std::string str_buf(decoded_req_target.substr(MAP_STORAGE_BASED.size() + 1));
                const model::Map* map_ptr = game.FindMap(util::Tagged<std::string, model::Map>(str_buf));
                if(map_ptr == nullptr)
                {
                    mode = HandleMode::HANDLE_ERR_MAP_NOT_FOUND;
                    ret = json_response(http::status::not_found, R"({"code": "mapNotFound", "message": "Map not found"})"sv, false, AllowedMethods::ALLOW_GET_HEAD);
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
        else if(!decoded_req_target.compare(0, JOIN_GAME_BASED.size(), JOIN_GAME_BASED))
        {
            if(req.method() != http::verb::post)
            {
                ret = json_response(http::status::method_not_allowed, R"({"code": "invalidMethod", "message": "Only POST method is expected"})"sv, true, AllowedMethods::ALLOW_POST);
            }
            else
            {
                try{
                
                std::string name = boost::json::value_to<std::string>(boost::json::parse(req.body()).at("userName"));
                std::string mapid = boost::json::value_to<std::string>(boost::json::parse(req.body()).at("mapId"));
                
                // try{
                    const model::Map* map_ptr = game.FindMap(util::Tagged<std::string, model::Map>(mapid));
                    if(name.length() == 0)
                    {
                        ret = json_response(http::status::bad_request, R"({"code": "invalidArgument", "message": "Invalid name"})"sv, true, AllowedMethods::ALLOW_POST);
                    }
                    else if(map_ptr == nullptr)
                    {
                        ret = json_response(http::status::not_found, R"({"code": "mapNotFound", "message": "Map not found"})"sv, true, AllowedMethods::ALLOW_POST);
                    }
                    else // вернуть токен
                    {
                        model::GameSession* session_ptr = game.FindGameSession(util::Tagged<std::string, model::Map>(mapid));
                        // try{
                            if(session_ptr == nullptr)
                            {
                            // std::cout << "NULL\n";
                                game.AddGameSession(util::Tagged<std::string, model::Map>(mapid));
                                session_ptr = game.FindGameSession(util::Tagged<std::string, model::Map>(mapid));
                                if(session_ptr == nullptr)
                                {
                                    throw;
                                }
                            }
                        
                        auto new_dog = session_ptr->AddDog(name);
                        // if(new_dog == nullptr)
                        //     std::cout << "dog null\n";
                        auto buf = players_.AddPlayer(session_ptr, new_dog);
                        
                        obj["authToken"] = buf.first;
                        obj["playerId"] = *buf.second->GetId();
                        std::string str;

                        str = (json::serialize(obj));
                        std::string_view sv_str(str);
                        ret = json_response(http::status::ok, sv_str, true, AllowedMethods::ALLOW_POST);

                    // ret = json_response(http::status::ok, R"({"authToken": "invalidArgument", "message": "Invalid name"})"sv, true);
                    }

                } catch (...)
                {


                    ret = json_response(http::status::bad_request, R"({"code": "invalidArgument", "message": "Join game request parse error"})"sv, true, AllowedMethods::ALLOW_POST);
                } 
            }
            
        }
        else if(!decoded_req_target.compare(0, PLAYERS_GAME_BASED.size(), PLAYERS_GAME_BASED))
        {
            if(req.method() == http::verb::get || req.method() == http::verb::head)
            {
                try{
                // std::cout << "IMA HERE_1\n";
                std::string_view local_bearer_buf = req.base().at(http::field::authorization);
                std::string received_token(local_bearer_buf.substr(7));
                // std::cout << "IMA HERE_2\n";
                model::Player* buffered_player = players_.FindByToken(received_token);
                // std::cout << "player ptr req:: " << std::hex << (uint64_t)buffered_player << std::endl;
                if(buffered_player == nullptr)
                {
                    ret = json_response(http::status::unauthorized, R"({"code": "unknownToken", "message": "Player token has not been found"})"sv, true, AllowedMethods::ALLOW_POST);
                }
                else
                {
                    // std::cout << "IMA HERE_3\n";
                    model::GameSession* session_ptr = buffered_player->GetSession();
                    // std::cout << "IMA HERE_4" << ":" << buffered_player->val << std::endl;;
                    // std::cout << "session ptr req:: " << std::hex << (uint64_t)session_ptr << std::endl;

                    auto dogs = session_ptr->GetDogs();

                    // std::cout << "IMA HERE_5\n";

                    for(int i = 0; i < dogs.size(); i++)
                    {   
                        // json::value local_json
                        json::object local_obj;
                        local_obj["name"] = dogs[i].GetName();
                        obj[std::to_string(*dogs[i].GetId())] = local_obj;
                    }
                    // std::cout << "IMA HERE_6\n";
                    std::string str;
                    str = (json::serialize(obj));
                    std::string_view sv_str(str);
                    ret = json_response(http::status::ok, std::move(sv_str), true, AllowedMethods::ALLOW_GET_HEAD);
                }

                // std::cout << received_token << std::endl;
                }
                catch(...)
                {
                    ret = json_response(http::status::unauthorized, R"({"code": "invalidToken", "message": "Auth not complete"})"sv, true, AllowedMethods::ALLOW_POST);
                }
            }
            else
            {
                ret = json_response(http::status::method_not_allowed, R"({"code": "invalidMethod", "message": "Invalid Method"})"sv, true, AllowedMethods::ALLOW_GET_HEAD);
            }
            

        }
        else
        {
            ret = json_response(http::status::bad_request, R"({"code": "badRequest", "message": "Bad Request"})"sv, true, AllowedMethods::ALLOW_POST); 
        }

        std::string str;
        if(mode == HandleMode::HANDLE_NEED_MAPS_LIST)
            str = (json::serialize(arr));
        else if(mode == HandleMode::HANDLE_NEED_MAP)
            str = (json::serialize(obj));
        
        std::string_view sv_str(str);


        switch(mode)
        {
            case (HandleMode::HANDLE_ERR_MAP_NOT_FOUND):
                // ret = json_response(http::status::not_found, R"({"code": "mapNotFound", "message": "Map not found"})"sv, true);
                break;
            case (HandleMode::HANDLE_NEED_MAPS_LIST):
                // ret = json_response(http::status::ok, sv_str);
                // break; 
            case (HandleMode::HANDLE_NEED_MAP):
                ret = json_response(http::status::ok, sv_str, false, AllowedMethods::ALLOW_GET_HEAD);
                break;
            case (HandleMode::HANDLE_ERR_BAD_REQUEST):
            default:
                /*"{\ncode: badRequest,\nmessage: BadRequest\n}"sv);*/
                break;
        }

        std::visit([sup_ptr = support_log.get()](auto&& args){
            sup_ptr->result = args.result();
            sup_ptr->result_int = args.result_int();
            sup_ptr->content_type = args.base().at(http::field::content_type);
        }, ret);


        
        // ret = text_response(http::status::not_found, R"(Page not found)"sv);
        return ret;
    }

    void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    
    strm << "{";
    auto ts = *rec[timestamp];
    strm << "\"timestamp\":" << "\"" << to_iso_extended_string(ts) << "\",";
    strm << "\"data\":" << rec[additional_data] << ",";
    // Выводим само сообщение.
    strm << "\"message\":" << "\"" << rec[expr::smessage] << "\"";
    strm << "}";
} 



}  // namespace http_handler

