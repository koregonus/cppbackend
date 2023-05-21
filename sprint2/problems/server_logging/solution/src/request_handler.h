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

#include <boost/json.hpp>
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
// namespace logging = boost::log;
// namespace keywords = boost::log::keywords;


using namespace std::literals;
// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде файла
using FileResponse = http::response<http::file_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

using var_t = std::variant<http::response<http::string_body>, http::response<http::file_body>>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
    constexpr static std::string_view APP_JSON = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};


// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, http::verb type,
                                  std::string_view content_type = ContentType::TEXT_PLAIN);

// StringResponse MakeStringResponseJSON(http::status status, std::string_view body, unsigned http_version,
//                                   bool keep_alive, http::verb type,
//                                   std::string_view content_type = ContentType::APP_JSON);

// StringResponse HandleRequest(StringRequest&& req);

// StringResponse HandleRequest(StringRequest&& req, model::Game& game);

std::string decode_uri(std::basic_string_view<char> target_from_target);
// {
//     std::string ret;
//     for(int i = 0; i < str_from_target.size(); i++)
//     {
//         if(str_from_target[i] == "%%")
//             std::cout << "pers!\n";
//     }
// }

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)


// {
//     // Выводить LineID стало проще.
//     strm << rec[line_id] << ": ";

//     // Момент времени приходится вручную конвертировать в строку.
//     // Для получения истинного значения атрибута нужно добавить
//     // разыменование. 
//     auto ts = *rec[timestamp];
//     strm << to_iso_extended_string(ts) << ": ";

//     // Выводим уровень, заключая его в угловые скобки.
//     strm << "<" << rec[logging::trivial::severity] << "> ";

//     // Выводим само сообщение.
//     strm << rec[expr::smessage];
// } 

struct SupportResponseLogger
{
    http::status result;
    std::string content_type;
    int result_int;
    // boost::asio::ip::address ip_addr;
};


class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    explicit RequestHandler(model::Game& game, std::basic_string<char> path_from_arg)
        : game_{game} {
        static_dir_path = fs::path(path_from_arg);
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    std::shared_ptr<struct SupportResponseLogger> operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                                                            boost::asio::ip::address addr, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        // struct SupportResponseLogger resp_buffer;

        // struct SupportResponseLogger support;
        std::shared_ptr<struct SupportResponseLogger> safe_support = std::make_shared<struct SupportResponseLogger>();
        // safe_support.get()->ip_addr = addr;

        send(std::move(HandleRequest(std::forward<decltype(req)>(req), game_, static_dir_path, safe_support)));
        // auto ret = HandleRequest(std::forward<decltype(req)>(req), game_, static_dir_path, safe_support);
        return safe_support;

    }



    var_t HandleRequest(StringRequest&& req, model::Game& game, fs::path& stat_folder, std::shared_ptr<struct SupportResponseLogger> supp);

private:
    model::Game& game_;
    fs::path static_dir_path;
};


class DurationMeasure {
public:
    DurationMeasure() = default;
    ~DurationMeasure() {
        ;
        // std::chrono::system_clock::time_point end_ts = std::chrono::system_clock::now();
        // std::cout << (end_ts - start_ts_).count() << std::endl;
    }
    std::chrono::milliseconds check_time()
    {
        std::chrono::system_clock::time_point cur_ts = std::chrono::system_clock::now();
        auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(cur_ts - start_ts_);
        // std::cout << (cur_ts - start_ts_).count() << std::endl;
        // auto debug_us = std::chrono::duration_cast<std::chrono::microseconds>(cur_ts - start_ts_);
        // std::cout << debug_us.count() << std::endl;
        return int_ms;
    }

private:
    std::chrono::system_clock::time_point start_ts_ = std::chrono::system_clock::now();
}; 

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)
// BOOST_LOG_ATTRIBUTE_KEYWORD(support_data, "SupportData", int)

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

template<class SomeRequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(SomeRequestHandler& RequestHandler)
        : decorated_(RequestHandler) {
        logging::add_common_attributes();
        logging::add_console_log( 
            std::clog,
            // keywords::format = "{[%TimeStamp%]: %Message%}",
            keywords::format = &MyFormatter,
            keywords::auto_flush = true
        ); 
    }

    template<typename Request>
    static void LogRequest(const Request& req, boost::asio::ip::address addr)
    {
        // std::cout << "log entry here\n";
        // std::cout << typeid(r).name() << std::endl;
        // std::cout << r.method() << std::endl;
        // std::count << r.target();
        json::value custom_data{{"ip"s, addr.to_string()}, {"URI"s, req.target()}, {"method"s, req.method_string()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "request received"sv;
        // BOOST_LOG_TRIVIAL(trace) << "Сообщение уровня trace"sv;
        // BOOST_LOG_TRIVIAL(debug) << "Сообщение уровня debug"sv;
        // BOOST_LOG_TRIVIAL(info) << "Сообщение уровня info"sv;
        // BOOST_LOG_TRIVIAL(warning) << "Сообщение уровня warning"sv;
        // BOOST_LOG_TRIVIAL(error) << "Сообщение уровня error"sv;
        // BOOST_LOG_TRIVIAL(fatal) << "Сообщение уровня fatal"sv;
    }

    template<typename Response>
    static void LogResponse(const Response& resp, boost::asio::ip::address addr, std::chrono::milliseconds resp_time)
    {
        // std::cout << rs_t.count() << std::endl;
        // std::cout << "log response here catched\n";
        // json::value custom_data{{"code"s, 0}};
        json::value custom_data{{"ip"s, addr.to_string()}, {"response_time"s, resp_time.count()}, 
                                {"code"s, resp.get()->result_int}, {"content_type"s, resp.get()->content_type}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "response sent"sv;
 
        // std::cout << r.get()->result << std::endl;
        // std::cout << r.get()->result_int << std::endl;
        // std::cout << r.get()->content_type << std::endl;
    }
public:

    // template<typename T>
    // auto operator() (T&& req) {
    //     std::cout << "decorated shit\n";
    //     auto resp = decorated_(std::move(std::forward<T>(req)));
    //     return resp;
    // }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, 
                    boost::asio::ip::address addr, Send&& send) {
        LogRequest(std::forward<decltype (req)>(req), addr);
        DurationMeasure g;
        auto resp = decorated_(std::forward<decltype (req)>(req), addr, std::forward<decltype (send)>(send));
        
        // auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(send));
        // std::shared_ptr<http::response<Body, Fields>> q;//, static_cast<Base2*>(p.get()));
        LogResponse(resp, addr, g.check_time());
    }
     // Response operator () (Request req) {
     //     Response resp = decorated_(std::move(req));
     //     LogResponse(resp);
     //     return resp;
     // }

private:
     SomeRequestHandler& decorated_;
};



}  // namespace http_handler



