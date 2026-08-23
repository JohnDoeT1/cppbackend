cmake_minimum_required(VERSION 3.15)
project(game_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS log log_setup json program_options)

add_executable(game_server
    src/main.cpp
    src/boost_json.cpp
    src/http_server.cpp
    src/json_loader.cpp
    src/model.cpp
    src/request_handler.cpp
    src/logging_request_handler.cpp
    src/ticker.cpp
)

target_include_directories(game_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_server
    Boost::boost
    Boost::log
    Boost::log_setup
    Boost::json
    Boost::program_options
)

find_package(Threads REQUIRED)
target_link_libraries(game_server Threads::Threads)
