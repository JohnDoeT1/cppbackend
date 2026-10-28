cmake_minimum_required(VERSION 3.16)
project(game_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Boost 1.81 REQUIRED COMPONENTS
    system program_options json log log_setup serialization
)

# <-- ДОБАВЛЕНО: libpqxx
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)

add_executable(game_server
    src/main.cpp
    src/model.cpp
    src/json_loader.cpp
    src/ticker.cpp
    src/collision_detector.cpp
    src/loot_generator.cpp
    src/http_server.cpp
)

target_include_directories(game_server PRIVATE src)

target_link_libraries(game_server PRIVATE
    Boost::system
    Boost::program_options
    Boost::json
    Boost::log
    Boost::log_setup
    Boost::serialization
    PkgConfig::PQXX           # <-- ДОБАВЛЕНО
)

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        find_package(Catch2 2 QUIET)
    endif()
    if(Catch2_FOUND)
        if(Catch2_VERSION VERSION_LESS 3)
            set(CATCH2_TARGET Catch2::Catch2)
        else()
            set(CATCH2_TARGET Catch2::Catch2WithMain)
        endif()
        add_executable(state_serialization_tests
            tests/state-serialization-tests.cpp
            src/model.cpp
            src/collision_detector.cpp
            src/loot_generator.cpp
        )
        target_include_directories(state_serialization_tests PRIVATE src)
        target_link_libraries(state_serialization_tests PRIVATE
            Boost::serialization
            ${CATCH2_TARGET}
        )
        add_test(NAME state_serialization_tests COMMAND state_serialization_tests)
    endif()
endif()
