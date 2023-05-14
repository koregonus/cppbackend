#include "http_server.h"

namespace http_server 
{
	// Разместите здесь реализацию http-сервера, взяв её из задания по разработке асинхронного сервера
	// template <typename Body, typename Fields>
	// void SessionBase::Write(http::response<Body, Fields>&& response) 
	// {
  //   // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
  //   auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));

  //   auto self = GetSharedThis();
  //   http::async_write(stream_, *safe_response,
  //                     [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
  //                     self->OnWrite(safe_response->need_eof(), ec, bytes_written);
  //                     });
  // }

	void ReportError(beast::error_code ec, std::string_view what) {
    	std::cerr << what << ": "sv << ec.message() << std::endl;
	}

	void SessionBase::Read()	{
    	// Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    	// request_ = {};
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
        HandleRequest(std::move(request_));
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
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
  }

}  // namespace http_server
