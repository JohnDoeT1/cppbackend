#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/program_options.hpp>
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
#include <optional>
#include <chrono>
#include <memory>          // <-- ДОБАВЛЕНО (std::unique_ptr)

#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "ticker.h"
#include "serializing_listener.h"   // <-- ДОБАВЛЕНО

using namespace std::literals;
namespace net = boost::asio;
namespace fs = std::filesystem;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace json = boost::json;
namespace po = boost::program_options;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

namespace {

struct Args {
    std::string config_file;
    std::string www_root;
    std::optional<int> tick_period;
    bool randomize_spawn_points = false;
    bool show_help = false;

    // <-- ДОБАВЛЕНО
    std::optional<fs::path> state_file;
    std::optional<int> save_state_period;  // в миллисекундах
};

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

std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    po::options_description desc{"Allowed options"s};
    
    Args args;
    
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<int>()->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.www_root)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions")
        // <-- ДОБАВЛЕНО
        ("state-file", po::value<std::string>()->value_name("path"s), "state file path")
        ("save-state-period", po::value<int>()->value_name("milliseconds"s), "save state period");
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    
    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }
    
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path is not specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files root is not specified"s);
    }
    
    if (vm.contains("tick-period"s)) {
        args.tick_period = vm["tick-period"s].as<int>();
    }

    // <-- ДОБАВЛЕНО: парсинг --state-file и --save-state-period
    if (vm.contains("state-file"s)) {
        args.state_file = fs::path{vm["state-file"s].as<std::string>()};
    }
    if (vm.contains("save-state-period"s)) {
        int period = vm["save-state-period"s].as<int>();
        if (period <= 0) {
            throw std::runtime_error("save-state-period must be positive"s);
        }
        args.save_state_period = period;
    }
    // --save-state-period игнорируется без --state-file
    if (!args.state_file) {
        args.save_state_period.reset();
    }
    
    return args;
}

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        InitLogging();
        
        auto args = ParseCommandLine(argc, argv);
        if (!args) {
            return EXIT_SUCCESS;
        }
        
        // Проверяем существование конфигурационного файла
        if (!fs::exists(args->config_file)) {
            std::cerr << "Error: Config file does not exist: " << args->config_file << std::endl;
            return EXIT_FAILURE;
        }
        
        // Проверяем существование статической директории
        if (!fs::exists(args->www_root) || !fs::is_directory(args->www_root)) {
            std::cerr << "Error: Static directory does not exist: " << args->www_root << std::endl;
            return EXIT_FAILURE;
        }
        
        // --- Загружаем конфигурацию игры ---
        auto loaded = json_loader::LoadGame(args->config_file);
        
        // Устанавливаем режим спавна
        loaded.game.SetRandomizeSpawnPoints(args->randomize_spawn_points);

        // ============================================================
        // <-- ДОБАВЛЕНО: восстановление состояния и создание слушателя
        // ============================================================
        std::unique_ptr<infrastructure::SerializingListener> save_listener;

        if (args->state_file) {
            // 1. Восстановление из файла, если он существует
            if (fs::exists(*args->state_file)) {
                try {
                    infrastructure::SerializingListener::Load(loaded.game, *args->state_file);
                } catch (const std::exception& e) {
                    json::value err_data{{"error", e.what()}};
                    BOOST_LOG_SEV(http_server::lg, boost::log::trivial::error)
                        << boost::log::add_value("AdditionalData", err_data)
                        << "failed to restore state";
                    return EXIT_FAILURE;
                }
            }

            // 2. Создаём слушателя
            save_listener = std::make_unique<infrastructure::SerializingListener>(
                loaded.game,
                *args->state_file,
                args->save_state_period
                    ? std::optional{std::chrono::milliseconds{*args->save_state_period}}
                    : std::nullopt);

            // 3. Синхронизируем накопленное время (если восстанавливались)
            save_listener->SetElapsed(
                std::chrono::milliseconds{loaded.game.GetElapsedMs()});

            // 4. Вешаем хук на Game::Tick
            loaded.game.SetTickListener(
                [&save_listener](std::chrono::milliseconds d) {
                    if (save_listener) {
                        save_listener->OnTick(d);
                    }
                });
        }
        // ============================================================

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int) {
            if (!ec) {
                ioc.stop();
            }
        });

        // --- Создаём обработчик запросов ---
        http_handler::RequestHandler handler{
            loaded.game,
            loaded.loot_types,
            args->www_root,
            !args->tick_period.has_value()
        };
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

        // Настраиваем тикер, если задан tick-period
        std::shared_ptr<Ticker> ticker;
        if (args->tick_period.has_value()) {
            auto api_strand = net::make_strand(ioc);
            ticker = std::make_shared<Ticker>(
                api_strand,
                std::chrono::milliseconds(*args->tick_period),
                [&loaded](std::chrono::milliseconds delta) {
                    loaded.game.Tick(static_cast<int>(delta.count()));
                }
            );
            ticker->Start();
        }

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        // ============================================================
        // <-- ДОБАВЛЕНО: финальное сохранение после остановки воркеров
        // ============================================================
        if (save_listener) {
            try {
                save_listener->Save();
            } catch (const std::exception& e) {
                json::value err_data{{"error", e.what()}};
                BOOST_LOG_SEV(http_server::lg, boost::log::trivial::error)
                    << boost::log::add_value("AdditionalData", err_data)
                    << "failed to save state on shutdown";
                return EXIT_FAILURE;
            }
        }
        // ============================================================
        
        // Логируем остановку сервера
        json::value exit_data{
            {"code", 0}
        };
        BOOST_LOG_SEV(http_server::lg, boost::log::trivial::info) 
            << boost::log::add_value("AdditionalData", exit_data)
            << "server exited";
        
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
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
