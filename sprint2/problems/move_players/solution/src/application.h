#pragma once


#include <string>
#include <optional>


#include "model.h"
// #include "request_handler.h"

#include <boost/asio.hpp>

#include <boost/json.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "http_req_resp_support.h"


namespace beast = boost::beast;
namespace http = beast::http;








// MakeStringResponse(http::status::unauthorized, R"({"code": "unknownToken", "message": "Player token has not been found"})"sv,
        						 // request.version(), request.keep_alive(), request.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);

namespace application {

std::optional<std::string> TryExtractToken(StringRequest& request);
	

// template <typename Fn>
// StringResponse ExecuteAuthorized(StringRequest& request, Fn&& action) 
// {
//     if (auto token = TryExtractToken(request)) {
//         return action(*token);
//     } else {
//         return MakeStringResponse(http::status::unauthorized, R"({"code": "invalidToken", "message": "Auth not complete"})"sv,
//         						 request.version(), request.keep_alive(), request.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
//     }
// }

	template <typename Fn>
	StringResponse ExecuteAuthorized(StringRequest& request, Fn&& action) 
	{
    	if (auto token = TryExtractToken(request)) {
        	return action(*token);
    	} else {
        	return MakeStringResponse(http::status::unauthorized, R"({"code": "invalidToken", "message": "Auth not complete"})"sv,
        						 request.version(), request.keep_alive(), request.method(), true, ContentType::APP_JSON, AllowedMethods::ALLOW_GET_HEAD);
    	}
	}

	using namespace std::literals;

	// // // Запрос, тело которого представлено в виде строки
	// using StringRequest = http::request<http::string_body>;
	// // // Ответ, тело которого представлено в виде строки
	// using StringResponse = http::response<http::string_body>;

	class ListMapUseCase {
		explicit ListMapUseCase(model::Game& game):game_{game} {}

	private:
		model::Game& game_;
	};
	

	class ApplicationFacade
	{
	public:
		explicit ApplicationFacade(model::Game& game, model::Players& players):game_{game}, 
																	 players_{players} {}

		StringResponse ListMap(StringRequest req);

		StringResponse PlayersList(StringRequest& req);
		
		StringResponse PlayersState(StringRequest& req);

		StringResponse SetPlayerAction(StringRequest& request);

	private:
		model::Game& game_;
		model::Players& players_;
	};

}