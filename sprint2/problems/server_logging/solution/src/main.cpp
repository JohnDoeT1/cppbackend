#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <filesystem>
#include <sstream>
#include <chrono>

#include "json_loader.h"
#include "request_handler.h"
#include "logging.h"

using namespace std::literals;
namespace net = boost::asio;
namespace fs = std::filesystem;
namespace http = boost::beast::http;

namespace {

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-dir>"sv << std::endl;
        return EXIT_FAILURE;
    }
    
    try {
        // Инициализируем логирование
        InitLogging();
        
        // Получаем пути из аргументов командной строки
        const std::string config_path = argv[1];
        const std::string static_path = argv[2];
        
        // Логируем запуск сервера
        std::ostringstream start_data;
        start_data << "{\"port\":8080,\"address\":\"0.0.0.0\"}";
        BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(additional_data, start_data.str())
            << "server started";
        
        // Проверяем существование конфигурационного файла
        if (!fs::exists(config_path)) {
            std::cerr << "Error: Config file does not exist: " << config_path << std::endl;
            return EXIT_FAILURE;
        }
        
        // Проверяем существование статической директории
        if (!fs::exists(static_path) || !fs::is_directory(static_path)) {
            std::cerr << "Error: Static directory does not exist: " << static_path << std::endl;
            return EXIT_FAILURE;
        }
        
        // Загружаем конфигурацию игры
        model::Game game = json_loader::LoadGame(config_path);

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int) {
            if (!ec) {
                ioc.stop();
            }
        });

        // Создаем обработчик запросов
        http_handler::RequestHandler handler{game, static_path};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;
        
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            // Логируем запрос
            std::ostringstream req_data;
            req_data << "{\"ip\":\"unknown\","
                     << "\"URI\":\"" << std::string(req.target()) << "\","
                     << "\"method\":\"" << std::string(req.method_string()) << "\"}";
            
            BOOST_LOG_TRIVIAL(info) 
                << logging::add_value(additional_data, req_data.str())
                << "request received";
            
            auto start = std::chrono::steady_clock::now();
            
            handler(std::forward<decltype(req)>(req), [send = std::forward<decltype(send)>(send), start](auto&& response) {
                auto end = std::chrono::steady_clock::now();
                auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                
                // Логируем ответ
                std::ostringstream resp_data;
                resp_data << "{\"ip\":\"unknown\","
                          << "\"response_time\":" << response_time << ","
                          << "\"code\":" << response.result_int() << ",";
                
                auto content_type = response.find(http::field::content_type);
                if (content_type != response.end()) {
                    resp_data << "\"content_type\":\"" << std::string(content_type->value()) << "\"";
                } else {
                    resp_data << "\"content_type\":null";
                }
                resp_data << "}";
                
                BOOST_LOG_TRIVIAL(info) 
                    << logging::add_value(additional_data, resp_data.str())
                    << "response sent";
                
                send(std::move(response));
            });
        });

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
        
        // Логируем остановку сервера
        std::ostringstream exit_data;
        exit_data << "{\"code\":0}";
        BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(additional_data, exit_data.str())
            << "server exited";
            
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return 0;
}
