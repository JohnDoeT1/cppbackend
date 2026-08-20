cmake_minimum_required(VERSION 3.15)
project(GameServer CXX)
set(CMAKE_CXX_STANDARD 20)

# Находим Boost в системе
find_package(Boost REQUIRED COMPONENTS 
    log 
    log_setup 
    json 
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
    Boost::json
    Boost::date_time
    Boost::thread
    Boost::filesystem
    Boost::system
)
