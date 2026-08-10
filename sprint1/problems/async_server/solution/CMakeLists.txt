cmake_minimum_required(VERSION 3.11)

project(hello_async)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

conan_basic_setup()

find_package(Boost 1.78 REQUIRED)

add_executable(
    hello_async
    src/main.cpp
    src/http_server.cpp
)

target_link_libraries(
    hello_async
    ${CONAN_LIBS}
)
