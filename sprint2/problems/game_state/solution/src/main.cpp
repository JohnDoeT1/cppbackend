#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <thread>
#include <filesystem>

#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace fs = std::filesystem;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

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

// Кастомный форматер для вывода JSON
void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = rec[timestamp];
    auto msg = rec[expr::smessage];
    auto data = rec[additional_data];
    
    json::object obj;
    if (ts) {
        std::ostringstream ss;
        ss << boost::posix_time::to_iso_extended_string(*ts);
        obj["timestamp"] = ss.str();
    }
    
    if (msg) {
        obj["message"] = msg.get();
    }
    
    if (data) {
        obj["data"] = *data;
    } else {
        obj["data"] = json::object{};
    }
    
    strm << json::serialize(obj);
}

void InitLogging() {
    logging::add_common_attributes();
    
    logging::add_console_log(
        std::cout,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-dir>"sv << std::endl;
        return EXIT_FAILURE;
    }
    
    try {
        InitLogging();
        
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
        http_handler::LoggingRequestHandler logging_handler{handler};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;
        
        // Логируем запуск сервера
        json::value start_data{
            {"port", static_cast<int>(port)},
            {"address", address.to_string()}
        };
        BOOST_LOG_SEV(http_server::lg, boost::log::trivial::info) 
            << boost::log::add_value("AdditionalData", start_data)
            << "server started";
        
        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
        
        // Логируем остановку сервера
        json::value exit_data{
            {"code", 0}
        };
        BOOST_LOG_SEV(http_server::lg, boost::log::trivial::info) 
            << boost::log::add_value("AdditionalData", exit_data)
            << "server exited";
        
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        // Логируем ошибку при выходе
        json::value exit_data{
            {"code", EXIT_FAILURE},
            {"exception", ex.what()}
        };
        BOOST_LOG_SEV(http_server::lg, boost::log::trivial::info) 
            << boost::log::add_value("AdditionalData", exit_data)
            << "server exited";
        
        return EXIT_FAILURE;
    }
}
