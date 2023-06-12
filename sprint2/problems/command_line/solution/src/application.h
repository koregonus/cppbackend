#pragma once

#include <string>
#include <optional>

#include "model.h"

#include <boost/asio.hpp>

#include <boost/json.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "http_req_resp_support.h"


namespace beast = boost::beast;
namespace http = beast::http;



namespace application {

std::optional<std::string> TryExtractToken(StringRequest& request);
	

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


	class ListMapUseCase {
		explicit ListMapUseCase(model::Game& game):game_{game} {}

	private:
		model::Game& game_;
	};
	

	class ApplicationFacade
	{
	public:
		explicit ApplicationFacade(model::Game& game, model::Players& players):game_{game}, 
																	 players_{players}, auto_timer_mode(false), random_spawn_dogs(false) {}

		StringResponse ListMap(StringRequest req);

		StringResponse PlayersList(StringRequest& req);
		
		StringResponse PlayersState(StringRequest& req);

		StringResponse SetPlayerAction(StringRequest& request);

		StringResponse TimerTick(StringRequest& request);

		void TimerTickAuto(std::chrono::milliseconds time_tick);

		void AutoTimerModeEnable();

		bool GetTimerMode();

		void SetRandomSpawn();

		bool IsInRandomSpawnMode();

	private:
		bool auto_timer_mode;
		bool random_spawn_dogs;
		model::Game& game_;
		model::Players& players_;
	};

}