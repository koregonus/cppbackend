#pragma once

#include <chrono>

#include <boost/asio.hpp>

#include <boost/json.hpp>

#include <optional>

#include "loot_generator.h"

namespace app_support	{

	namespace json = boost::json;

	using namespace std::literals;

	namespace net = boost::asio;
	namespace sys = boost::system;

	class Ticker : public std::enable_shared_from_this<Ticker> {
	public:
	    using Strand = net::strand<net::io_context::executor_type>;
	    using Handler = std::function<void(std::chrono::milliseconds delta)>;

	    Ticker(Strand strand, std::chrono::milliseconds period, bool start, Handler handler):strand_{strand}, period_{period},
	                                                                                        handler_{handler}, enabled(start){}

	    void Start() {
	        if(!enabled)
	        {
	            return;
	        }
	        last_tick_ = std::chrono::steady_clock::now();
	        auto handle = [self = shared_from_this()](){
	            self->ScheduleTick();
	        };
	        net::dispatch(strand_, handle);
	    }
	private:
	    bool enabled;
	    void ScheduleTick() {
	        /* выполнить OnTick через промежуток времени period_ */
	        timer_.expires_after(period_);
	        timer_.async_wait([self = shared_from_this()](sys::error_code ec){
	            self->OnTick(ec);
	        });
	        
	    }
	    void OnTick(sys::error_code ec) {
	        auto current_tick = std::chrono::steady_clock::now();
	        handler_(std::chrono::duration_cast<std::chrono::milliseconds>(current_tick - last_tick_));
	        last_tick_ = current_tick;
	        ScheduleTick();
	    }

	    Strand strand_;
	    net::steady_timer timer_{strand_};
	    std::chrono::milliseconds period_;
	    Handler handler_;
	    std::chrono::steady_clock::time_point last_tick_;
	};

	struct loottypes_for_maps
	{
		int count;
		std::shared_ptr<json::array> lootTypes;
	};

	class FrontendExtraDataMap
	{
	public:
		FrontendExtraDataMap(){}

		std::optional<std::shared_ptr<loottypes_for_maps>> GetExtraData(std::string Id);

		void AddExtraData(std::string Id, std::shared_ptr<loottypes_for_maps> local_loot);

	private:
		std::map<std::string, std::shared_ptr<loottypes_for_maps>> extra_data_;
	};
}