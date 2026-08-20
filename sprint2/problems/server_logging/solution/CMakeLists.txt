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

add_executable(game_server 
    src/main.cpp
    src/boost_json.cpp
    src/http_server.cpp
    src/json_loader.cpp
    src/model.cpp
    src/request_handler.cpp
)

target_include_directories(game_server PRIVATE src)
target_link_libraries(game_server PRIVATE 
    Boost::log
    Boost::log_setup
    Boost::date_time
    Boost::thread
    Boost::filesystem
    Boost::system
)
