#include "http_server.h"


BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

namespace http_server 
{
  namespace sys = boost::system;
  namespace http = boost::beast::http;
  namespace logging = boost::log;
  namespace sinks = boost::log::sinks;
  namespace keywords = boost::log::keywords;
  namespace expr = boost::log::expressions;
  namespace attrs = boost::log::attributes;
	// Разместите здесь реализацию http-сервера, взяв её из задания по разработке асинхронного сервера

	void ReportError(beast::error_code ec, std::string_view what) {
      json::value custom_data{{"code"s, ec.value()}, {"text"s, ec.message()}, {"where"s, what}};
      BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                            << "error"sv;
	}

	void SessionBase::Read()	{
    	// Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    	request_.clear();
    	stream_.expires_after(30s);
    	// Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    	http::async_read(stream_, buffer_, request_,
      					// По окончании операции будет вызван метод OnRead
                    	beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
	}


	void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
	}

	void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    if (ec == http::error::end_of_stream) 
    {
    // Нормальная ситуация - клиент закрыл соединение
    	return Close();
    }
    if (ec) {
    	return ReportError(ec, "read"sv);
    }
        HandleRequest(std::move(request_), stream_.socket().remote_endpoint());
  }

	void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written)	{
    	if (ec) {
    		return ReportError(ec, "write"sv);
    	}

    	if (close) {
            // Семантика ответа требует закрыть соединение
    		return Close();
    	}
      		// Считываем следующий запрос
      	Read();
	}

  void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
  }

}  // namespace http_server
