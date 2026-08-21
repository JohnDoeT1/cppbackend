cmake_minimum_required(VERSION 3.15)
project(game_server CXX)

# Настройка C++ стандарта
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск Boost через Conan
find_package(Boost REQUIRED COMPONENTS log log_setup json)

# Добавляем исполняемый файл
add_executable(game_server
    src/main.cpp
    src/boost_json.cpp
    src/http_server.cpp
    src/json_loader.cpp
    src/model.cpp
    src/request_handler.cpp
    src/logging_request_handler.cpp
)

# Подключаем заголовочные файлы
target_include_directories(game_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Линкуем библиотеки Boost
target_link_libraries(game_server
    Boost::boost
    Boost::log
    Boost::log_setup
    Boost::json
)

# Добавляем поддержку потоков
find_package(Threads REQUIRED)
target_link_libraries(game_server Threads::Threads)
