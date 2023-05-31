#pragma once
#include "http_server.h"
#include "model.h"
#include "json_loader.h"
#include <filesystem>
#include <variant>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <boost/asio.hpp>

#include <boost/json.hpp>

#include "application.h"

#include "http_req_resp_support.h"

namespace json = boost::json;

using namespace std::literals;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

namespace fs = std::filesystem;
namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace net = boost::asio;
// namespace logging = boost::log;
// namespace keywords = boost::log::keywords;


using namespace std::literals;
// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде файла
using FileResponse = http::response<http::file_body>;
// Ответ, тело которого представлено пустым
using EmptyResponse = http::response<http::empty_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

using var_t = std::variant<http::response<http::string_body>, http::response<http::file_body>>;


constexpr static std::string_view MAP_STORAGE_BASED = "/api/v1/maps"sv;
constexpr static std::string_view JOIN_GAME_BASED = "/api/v1/game/join"sv;
constexpr static std::string_view PLAYERS_GAME_BASED = "/api/v1/game/players"sv;
constexpr static std::string_view PLAYERS_STATE_BASED = "/api/v1/game/state"sv;
constexpr int API_PATH_LEN = 5;




// Создаёт StringResponse с заданными параметрами
// StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
//                                   bool keep_alive, http::verb type, bool cache_control,
//                                   std::string_view content_type = ContentType::TEXT_PLAIN,
//                                   std::string_view allowed_methods = AllowedMethods::ALLOW_GET_HEAD);


std::string decode_uri(std::basic_string_view<char> target_from_target);


BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)


struct SupportResponseLogger
{
    http::status result;
    std::string content_type;
    int result_int;
    // boost::asio::ip::address ip_addr;
};


class RequestHandler: public std::enable_shared_from_this<RequestHandler> {
public:

    enum class HandleMode {
        HANDLE_UNKNOWN_MODE,
        HANDLE_NEED_MAPS_LIST,
        HANDLE_NEED_MAP,
        HANDLE_NEED_API,
        HANDLE_NEED_STATIC_CONTENT,
        HANDLE_ERR_MAP_NOT_FOUND,
        HANDLE_ERR_PAGE_NOT_FOUND,
        HANDLE_ERR_BAD_REQUEST
    };

    using Strand = net::strand<net::io_context::executor_type>;
    
    // explicit RequestHandler(model::Game& game)
    //     : game_{game} {
    // }


    explicit RequestHandler(model::Game& game, model::Players& players, application::ApplicationFacade& application, Strand api_strand, std::basic_string<char> path_from_arg)
        : game_{game}, players_{players}, app_{application}, api_strand_{api_strand} {
        static_dir_path = fs::path(path_from_arg);
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    std::shared_ptr<struct SupportResponseLogger> operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                                                            boost::asio::ip::tcp::endpoint endpoint, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        // struct SupportResponseLogger resp_buffer;

        // struct SupportResponseLogger support;
        std::shared_ptr<struct SupportResponseLogger> safe_support = std::make_shared<struct SupportResponseLogger>();
        // safe_support.get()->ip_addr = addr;
        std::string decoded_req_target = decode_uri(req.target());

        if(!decoded_req_target.compare(0, 4, MAP_STORAGE_BASED, 0, 4))
        {
            // std::cout << "compare!\n";
                    // mode = HandleMode::HANDLE_ERR_BAD_REQUEST; // currently it's bad request
            auto handle = [self = shared_from_this(), send, req = std::forward<decltype(req)>(req), safe_support](){
                // try {
                        // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(self->api_strand_.running_in_this_thread());
                        send((self->HandleApiRequest(req, self->game_, self->players_, self->app_,self->static_dir_path, safe_support)));
                    // } catch (...) 
                    // {
                        // std::cout << "err << std::endl;"
                        // ;
            //             send(self->ReportServerError(version, keep_alive));
                    // }
            };

            net::dispatch(api_strand_, handle);
        }
        else
            send(std::move(HandleRequest(std::forward<decltype(req)>(req), game_, players_, app_, static_dir_path, safe_support)));

        return safe_support;

    }



    var_t HandleRequest(StringRequest&& req, model::Game& game, model::Players& players, 
                        application::ApplicationFacade& AppFacade, fs::path& stat_folder, std::shared_ptr<struct SupportResponseLogger> supp);
    var_t HandleApiRequest(StringRequest req, model::Game& game, model::Players& players,
                            application::ApplicationFacade& AppFacade, fs::path& stat_folder, std::shared_ptr<struct SupportResponseLogger> supp);

private:
    using FileRequestResult = std::variant<EmptyResponse, StringResponse, FileResponse>;

    model::Game& game_;
    model::Players& players_;
    application::ApplicationFacade app_;
    fs::path static_dir_path;
    Strand api_strand_;
};


class DurationMeasure {
public:
    DurationMeasure() = default;
    ~DurationMeasure() {}
    std::chrono::milliseconds check_time()
    {
        std::chrono::system_clock::time_point cur_ts = std::chrono::system_clock::now();
        auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(cur_ts - start_ts_);
        return int_ms;
    }

private:
    std::chrono::system_clock::time_point start_ts_ = std::chrono::system_clock::now();
}; 

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

template<class SomeRequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(SomeRequestHandler& RequestHandler)
        : decorated_(RequestHandler) {
        logging::add_common_attributes();
        logging::add_console_log( 
            std::cout,
            keywords::format = &MyFormatter,
            keywords::auto_flush = true
        ); 
    }

    template<typename Request>
    static void LogRequest(const Request& req, boost::asio::ip::tcp::endpoint endpoint)
    {
        json::value custom_data{{"ip"s, endpoint.address().to_string()}, {"URI"s, req.target()}, {"method"s, req.method_string()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "request received"sv;
    }

    template<typename Response>
    static void LogResponse(const Response& resp, boost::asio::ip::tcp::endpoint endpoint, std::chrono::milliseconds resp_time)
    {
        json::value custom_data{{"ip"s, endpoint.address().to_string()}, {"response_time"s, resp_time.count()}, 
                                {"code"s, resp.get()->result_int}, {"content_type"s, resp.get()->content_type}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "response sent"sv;
    }
    public:

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, 
                    boost::asio::ip::tcp::endpoint endpoint, Send&& send) {
        LogRequest(std::forward<decltype (req)>(req), endpoint);
        DurationMeasure g;
        auto resp = decorated_(std::forward<decltype (req)>(req), endpoint, std::forward<decltype (send)>(send));

        LogResponse(resp, endpoint, g.check_time());
    }


private:
     SomeRequestHandler& decorated_;
};



}  // namespace http_handler



