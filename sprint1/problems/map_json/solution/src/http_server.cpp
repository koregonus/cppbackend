#include "http_server.h"

namespace http_server {

// Разместите здесь реализацию http-сервера, взяв её из задания по разработке асинхронного сервера

	void ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": "sv << ec.message() << std::endl;
	}


	void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
	}


	// // Создаёт StringResponse с заданными параметрами
	// StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
  //                                 bool keep_alive, http::verb type,
  //                                 std::string_view content_type/* = ContentType::TEXT_HTML*/) {
  //   StringResponse response(status, http_version);
  //   response.set(http::field::content_type, content_type);
  //   if(type != http::verb::head)
  //       response.body() = body;
  //   if(status != http::status::ok)
  //       response.set(http::field::allow, "GET, HEAD"sv);
  //   response.content_length(body.size());
  //   response.keep_alive(keep_alive);
  //   return response;
	// }

// 	StringResponse HandleRequest(StringRequest&& req) {
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

}  // namespace http_server
