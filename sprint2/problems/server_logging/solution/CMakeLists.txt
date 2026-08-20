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

# Собираем Boost.JSON из исходников
add_library(boost_json STATIC
    /usr/local/src/json-boost-1.78.0/src/src.cpp
)
target_include_directories(boost_json PUBLIC /usr/local/src/json-boost-1.78.0/include)
set_target_properties(boost_json PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(game_server 
    src/main.cpp
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
    boost_json
)
