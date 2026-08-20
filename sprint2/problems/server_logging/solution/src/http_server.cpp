#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>
#include <exception>

namespace http_server {

void SessionBase::Run() {
    Read();
}

void SessionBase::Read() {
    request_ = {};
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

    try {
        HandleRequest(std::move(request_));
    } catch (const std::exception& ex) {
        ReportError(beast::error_code(1, beast::generic_category()), ex.what());
        http::response<http::string_body> res(http::status::internal_server_error, 11);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Internal Server Error";
        res.content_length(res.body().size());
        res.keep_alive(false);
        Write(std::move(res));
    } catch (...) {
        ReportError(beast::error_code(1, beast::generic_category()), "unknown error");
        http::response<http::string_body> res(http::status::internal_server_error, 11);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Internal Server Error";
        res.content_length(res.body().size());
        res.keep_alive(false);
        Write(std::move(res));
    }
}

void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t) {
    if (ec) {
        return ReportError(ec, "write");
    }
    if (close) {
        return Close();
    }

    try {
        Read();
    } catch (const std::exception& ex) {
        ReportError(beast::error_code(1, beast::generic_category()), ex.what());
        Close();
    } catch (...) {
        ReportError(beast::error_code(1, beast::generic_category()), "unknown error");
        Close();
    }
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
