#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

void SessionBase::Run() {
    Read();
}

void SessionBase::Read() {
    request_ = {};
    buffer_.consume(buffer_.size());  // очищаем буфер перед чтением
    stream_.expires_after(std::chrono::seconds(30));

    http::async_read(
        stream_,
        buffer_,
        request_,
        beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis())
    );
}

void SessionBase::OnRead(beast::error_code ec, std::size_t) {
    if (ec == http::error::end_of_stream) {
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read");
    }

    // Обрабатываем запрос. Если обработчик бросит исключение, закроем соединение.
    HandleRequest(std::move(request_));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t) {
    if (ec) {
        return ReportError(ec, "write");
    }
    if (close) {
        return Close();
    }
    Read();
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        ReportError(ec, "shutdown");
    }
}

void SessionBase::ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": " << ec.message() << std::endl;
}

} // namespace http_server
