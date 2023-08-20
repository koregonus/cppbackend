
#include "http_req_resp_support.h"

// #include <iostream>
#include <iomanip>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>


namespace beast = boost::beast;
namespace http = beast::http;

namespace req_resp_support
{

    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                     bool keep_alive, http::verb type, bool cache_control,
                                     std::string_view content_type, std::string_view allowed_method) 
    {
           StringResponse response(status, http_version);
           response.set(http::field::content_type, content_type);
           if(type != http::verb::head)
                response.body() = body;
            if(status == http::status::method_not_allowed)
                response.set(http::field::allow, allowed_method);
            if(cache_control)
                response.set(http::field::cache_control, "no-cache");
            response.content_length(body.size());
            response.keep_alive(keep_alive);
        return response;
    }

} // namespace req_resp_support