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

#include "postgres.h"
#include <pqxx/connection>


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
		explicit ApplicationFacade(model::Game& game, model::Players& players, postgres::ConnectionPool& conn,
																app_support::FrontendExtraDataMap& extra_data):game_{game}, players_{players},
																frontend_extra_data_{extra_data}, auto_timer_mode(false), random_spawn_dogs(false), conn_pool_(conn)
																{
																	try{
																		auto connection = conn_pool_.GetConnection();
																		postgres::CreateTable(*connection);
																	}
																	catch(...)
																	{
																		std::cout << "db init error\n";
																	}
																}

		StringResponse ListMap(const StringRequest req);

		StringResponse PlayersList(const StringRequest& req);
		
		StringResponse PlayersState(const StringRequest& req);

		StringResponse SetPlayerAction(const StringRequest& request);

		StringResponse TimerTick(const StringRequest& request);
		
		StringResponse GetRecordsFromDB(const StringRequest& req, int start, int max_items);

		void TimerTickAuto(std::chrono::milliseconds time_tick);

		void AutoTimerModeEnable();

		bool GetTimerMode() const;

		void SetRandomSpawn();

		bool IsInRandomSpawnMode() const;

		void RestoreGame();

    void BackupGame();

    void TryBackupTimer(int tick_ms);

		std::optional<std::shared_ptr<app_support::loottypes_for_maps>> GetExtraDataMap(std::string Id) const noexcept;

		void InitBackupParams(std::string& path, int period);
	private:
		bool auto_timer_mode;
		bool random_spawn_dogs;
		std::string backup_path;
		int backup_period = 0;
		int prev_backup_period = 0;
		model::Game& game_;
		model::Players& players_;
		app_support::FrontendExtraDataMap& frontend_extra_data_;
		postgres::ConnectionPool& conn_pool_;
	};

}