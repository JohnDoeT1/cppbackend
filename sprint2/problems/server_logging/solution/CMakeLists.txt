cmake_minimum_required(VERSION 3.15)
project(GameServer CXX)
set(CMAKE_CXX_STANDARD 20)

# Находим Boost в системе (без json, так как его нет в 1.74)
find_package(Boost REQUIRED COMPONENTS 
    log 
    log_setup 
    date_time 
    thread 
    filesystem 
    system
)

add_executable(game_server 
    main.cpp
)

# Линкуем компоненты Boost
target_link_libraries(game_server PRIVATE 
    Boost::log
    Boost::log_setup
    Boost::date_time
    Boost::thread
    Boost::filesystem
    Boost::system
)
