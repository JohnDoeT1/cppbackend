cmake_minimum_required(VERSION 3.15)

project(collision_detection CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Подмена имени функции через -DFindGatherEvents=FindGatherEvents_WrongN.
# При обычной сборке переменная пустая -> макрос НЕ добавляем.
if(DEFINED FindGatherEvents AND NOT FindGatherEvents STREQUAL "")
    add_compile_definitions(FindGatherEvents=${FindGatherEvents})
endif()

find_package(Threads REQUIRED)

add_library(collision_detection_lib STATIC
    src/collision_detector.cpp
)
target_include_directories(collision_detection_lib PUBLIC src)
target_link_libraries(collision_detection_lib PUBLIC Threads::Threads)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(collision_detection_tests
    tests/collision-detector-tests.cpp
)
target_link_libraries(collision_detection_tests PRIVATE
    Catch2::Catch2WithMain
    collision_detection_lib
)

include(CTest)
include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(collision_detection_tests)
