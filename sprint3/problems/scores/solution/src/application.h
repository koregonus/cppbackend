#pragma once

#include <string>
#include <optional>

#include "model.h"

#include <boost/asio.hpp>

#include <boost/json.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "http_req_resp_support.h"

#include "application_support.h"


namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;


namespace application {

std::optional<std::string> TryExtractToken(const StringRequest& request);
	

	template <typename Fn>
	StringResponse ExecuteAuthorized(const StringRequest& request, Fn&& action) 
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
		explicit ApplicationFacade(model::Game& game, model::Players& players,
																app_support::FrontendExtraDataMap& extra_data):game_{game}, players_{players},
																frontend_extra_data_{extra_data}, auto_timer_mode(false), random_spawn_dogs(false) {}

		StringResponse ListMap(const StringRequest req);

		StringResponse PlayersList(const StringRequest& req);
		
		StringResponse PlayersState(const StringRequest& req);

		StringResponse SetPlayerAction(const StringRequest& request);

		StringResponse TimerTick(const StringRequest& request);

		void TimerTickAuto(std::chrono::milliseconds time_tick);

		void AutoTimerModeEnable();

		bool GetTimerMode() const;

		void SetRandomSpawn();

		bool IsInRandomSpawnMode() const;

		std::optional<std::shared_ptr<app_support::loottypes_for_maps>> GetExtraDataMap(std::string Id) const noexcept;

	private:
		bool auto_timer_mode;
		bool random_spawn_dogs;
		model::Game& game_;
		model::Players& players_;
		app_support::FrontendExtraDataMap& frontend_extra_data_;
	};

}