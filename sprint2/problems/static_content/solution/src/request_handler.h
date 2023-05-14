#pragma once
#include "http_server.h"
#include "model.h"
#include "json_loader.h"
#include <filesystem>
#include <variant>

namespace fs = std::filesystem;
namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;


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
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send

        send(std::move(HandleRequest(std::forward<decltype(req)>(req), game_, static_dir_path)));
    }

    var_t HandleRequest(StringRequest&& req, model::Game& game, fs::path& stat_folder);

private:
    model::Game& game_;
    fs::path static_dir_path;
};

}  // namespace http_handler
