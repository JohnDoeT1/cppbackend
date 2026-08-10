cmake_minimum_required(VERSION 3.11)

project(Hello CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Явно задаём build type, если не передан через cmake
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
    include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

find_package(Boost 1.78.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

find_package(Threads REQUIRED)

add_executable(hello main.cpp)

target_link_libraries(hello PRIVATE Threads::Threads)
