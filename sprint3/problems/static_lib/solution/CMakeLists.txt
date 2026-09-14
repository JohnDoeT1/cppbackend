cmake_minimum_required(VERSION 3.15)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTING "Build unit tests" ON)

find_package(Boost 1.74 REQUIRED COMPONENTS json system log program_options)
find_package(Threads REQUIRED)

# --- Статическая библиотека с игровой моделью ---
add_library(game_model STATIC
    src/model.cpp
    src/loot_generator.cpp
    src/json_loader.cpp
)

# Публичный интерфейс: заголовки + зависимости,
# нужные для компиляции потребителей.
target_include_directories(game_model PUBLIC src)
target_link_libraries(game_model PUBLIC
    Boost::json
    Boost::system
    Threads::Threads
)

# --- Основной сервер (сетевой код + main) ---
add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/ticker.cpp
)

target_link_libraries(game_server PRIVATE
    game_model
    Boost::log
    Boost::program_options
)

# --- Тесты (Catch2) ---
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(game_server_tests
        tests/loot_generator_tests.cpp
        tests/model_tests.cpp
    )
    target_link_libraries(game_server_tests PRIVATE
        Catch2::Catch2WithMain
        game_model
    )

    # --- Интеграция с CTest (из задания) ---
    include(CTest)
    include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
    catch_discover_tests(game_server_tests)
endif()
