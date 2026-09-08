#pragma once

#include "sdk.h"

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <memory>
#include <utility>
#include <string_view>

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace beast = boost::beast;
namespace http = beast::http;

// Базовый класс сессии обработки HTTP-запросов
class SessionBase {
public:
    using HttpRequest = http::request<http::string_body>;

    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;

    // Запускает асинхронную обработку сессии
    void Run();

protected:
    // Конструктор принимает сокет для обмена данными с клиентом
    explicit SessionBase(tcp::socket&& socket)
        : stream_(std::move(socket)) {
    }

    virtual ~SessionBase() = default;

    // Отправляет HTTP-ответ клиенту
    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));

        http::async_write(
            stream_,
            *safe_response,
            [self = GetSharedThis(), safe_response](beast::error_code ec, std::size_t bytes_written) {
                self->OnWrite(safe_response->need_eof(), ec, bytes_written);
            }
        );
    }

private:
    // Читает HTTP-запрос из сокета
    void Read();

    // Обрабатывает завершение чтения запроса
    void OnRead(beast::error_code ec, std::size_t bytes_read);

    // Обрабатывает завершение записи ответа
    void OnWrite(bool close, beast::error_code ec, std::size_t bytes_written);

    // Закрывает соединение с клиентом
    void Close();

    // Логирует ошибки
    void ReportError(beast::error_code ec, std::string_view what);

    // Обрабатывает полученный запрос (реализуется в наследниках)
    virtual void HandleRequest(HttpRequest&& request) = 0;

    // Возвращает shared_ptr на себя (реализуется в наследниках)
    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
};

// Конкретная реализация сессии с обработчиком запросов
template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& handler)
        : SessionBase(std::move(socket)), request_handler_(std::forward<Handler>(handler)) {
    }

private:
    std::shared_ptr<SessionBase> GetSharedThis() override {
        return this->shared_from_this();
    }

    void HandleRequest(HttpRequest&& request) override {
        request_handler_(
            std::move(request),
            [self = this->shared_from_this()](auto&& response) {
                self->Write(std::move(response));
            }
        );
    }

    RequestHandler request_handler_;
};

// Слушатель входящих подключений
template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint, RequestHandler handler)
        : acceptor_(ioc), request_handler_(std::move(handler)) {
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
    }

    // Запускает прослушивание входящих подключений
    void Run() {
        DoAccept();
    }

private:
    // Асинхронно принимает входящее подключение
    void DoAccept() {
        acceptor_.async_accept(
            [self = this->shared_from_this()](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    self->AsyncRunSession(std::move(socket));
                }
                self->DoAccept();
            }
        );
    }

    // Создаёт и запускает сессию для нового подключения
    void AsyncRunSession(tcp::socket&& socket) {
        std::make_shared<Session<RequestHandler>>(
            std::move(socket),
            request_handler_
        )->Run();
    }

    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

// Запускает HTTP-сервер на указанном endpoint
template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;
    std::make_shared<MyListener>(
        ioc,
        endpoint,
        std::forward<RequestHandler>(handler)
    )->Run();
}

} // namespace http_server
