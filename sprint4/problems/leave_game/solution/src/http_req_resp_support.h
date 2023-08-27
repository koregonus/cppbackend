#pragma once


#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>




namespace beast = boost::beast;
namespace http = beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

namespace req_resp_support
{
    using namespace std::literals;
    
    struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
    constexpr static std::string_view APP_JSON = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
    };

    struct AllowedMethods {
        AllowedMethods() = delete;
        constexpr static std::string_view ALLOW_GET_HEAD = "GET, HEAD"sv;
        constexpr static std::string_view ALLOW_POST = "POST"sv;
    };


    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                      bool keep_alive, http::verb type, bool cache_control,
                                      std::string_view content_type = ContentType::TEXT_PLAIN,
                                      std::string_view allowed_methods = AllowedMethods::ALLOW_GET_HEAD);
}



