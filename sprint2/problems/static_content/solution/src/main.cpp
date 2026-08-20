#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/address.hpp>
#include <iostream>
#include <thread>
#include <filesystem>

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace fs = std::filesystem;

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
        // Получаем пути из аргументов командной строки
        const std::string config_path = argv[1];
        const std::string static_path = argv[2];
        
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

        // Создаем обработчик запросов с путём к статическим файлам
        http_handler::RequestHandler handler{game, static_path};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;
        
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        std::cout << "Server has started..."sv << std::endl;

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
