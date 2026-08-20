cmake_minimum_required(VERSION 3.15)
project(GameServer CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS 
    log 
    log_setup 
    date_time 
    thread 
    filesystem 
    system
)

# Скачиваем Boost.JSON (header-only)
include(FetchContent)
FetchContent_Declare(
    boost_json
    URL https://github.com/boostorg/json/archive/refs/tags/boost-1.78.0.tar.gz
)
FetchContent_GetProperties(boost_json)
if(NOT boost_json_POPULATED)
    FetchContent_Populate(boost_json)
endif()

add_executable(game_server 
    src/main.cpp
    src/http_server.cpp
    src/json_loader.cpp
    src/model.cpp
    src/request_handler.cpp
)

target_include_directories(game_server PRIVATE 
    src
    ${boost_json_SOURCE_DIR}/include
)

target_link_libraries(game_server PRIVATE 
    Boost::log
    Boost::log_setup
    Boost::date_time
    Boost::thread
    Boost::filesystem
    Boost::system
)
