cmake_minimum_required(VERSION 3.15)

project(tv CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Catch2 ---
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

# --- GoogleTest ---
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# На Linux это не нужно, но не помешает для переносимости:
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- Boost.Test ---
# Boost через FetchContent не тянем — слишком тяжёлая зависимость.
# Используем системный Boost, если он установлен.
find_package(Boost 1.74 REQUIRED COMPONENTS unit_test_framework)

# --- Основное приложение ---
add_executable(app
    src/main.cpp
    src/tv.h
    src/menu.h
    src/controller.h
)

# --- Тесты на Catch2 ---
add_executable(catch_tv_tests
    tests/catch_tv_tests.cpp
    tests/catch_controller_tests.cpp
    src/tv.h
    src/menu.h
    src/controller.h
)
target_link_libraries(catch_tv_tests PRIVATE Catch2::Catch2WithMain)

# --- Тесты на GoogleTest ---
add_executable(google_tv_tests
    tests/google_tv_tests.cpp
    tests/google_controller_tests.cpp
    src/tv.h
    src/menu.h
    src/controller.h
)
target_link_libraries(google_tv_tests PRIVATE GTest::gtest GTest::gmock GTest::gtest_main)

# --- Тесты на Boost.Test ---
add_executable(boost_tv_tests
    tests/boost_tv_tests.cpp
    tests/boost_controller_tests.cpp
    tests/boost_test_helpers.h
    src/tv.h
    src/menu.h
    src/controller.h
)
target_link_libraries(boost_tv_tests PRIVATE Boost::unit_test_framework)
