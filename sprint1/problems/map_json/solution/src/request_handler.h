#pragma once
#include "http_server.h"
#include "model.h"
#include "json_loader.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;
// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;


struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APP_JSON = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};


// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, http::verb type,
                                  std::string_view content_type = ContentType::TEXT_HTML);

StringResponse MakeStringResponseJSON(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, http::verb type,
                                  std::string_view content_type = ContentType::APP_JSON);

StringResponse HandleRequest(StringRequest&& req);

StringResponse HandleRequest(StringRequest&& req, model::Game& game);


// StringResponse HandleRequest(StringRequest&& req); //{
//     const auto text_response = [&req](http::status status, std::string_view text) {
//         return MakeStringResponse(status, text, req.version(), req.keep_alive(), req.method());
//     };
//     if(req.method() == http::verb::get || req.method() == http::verb::head)
//     {
//         std::string str = "Hello"; //</strong>
//         if(req.target().substr(1).size() > 0)
//         {
//             str.append(", ");
//             str.append(req.target().substr(1));
//         }
//         // str.append("</strong>");
//         std::string_view sv_str(str);
//         // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
//         return text_response(http::status::ok, sv_str/*"<strong>Hello</strong>"sv*/);
//     }
//     else
//     {
//         return text_response(http::status::method_not_allowed, "Invalid method"sv);
//     }
    
// }

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        // const model::Game&& game_ref = game_;
        send(std::move(HandleRequest(std::forward<decltype(req)>(req), game_)));

        // auto map = game_.FindMap(util::Tagged<std::string, model::Map>("map1"));
        // std::cout << map->GetName() << std::endl;

        // auto buf = map->GetOffices();

        // std::cout << buf[0].GetPosition().x << std::endl;
        // std::cout << buf[0].GetPosition().y << std::endl;

        // std::cout << "whzoooh!\n";
        // StringResponse resp = HandleRequest(std::forward<decltype(req)>(req));
        // send(std::move(resp));
        // send = HandleRequest(std::forward<decltype(req)>(req));
        // std::cout << resp << std::endl;
    }

private:
    model::Game& game_;
};

}  // namespace http_handler
