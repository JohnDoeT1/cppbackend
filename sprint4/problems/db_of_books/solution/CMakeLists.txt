cmake_minimum_required(VERSION 3.15)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conan_toolchain.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/conan_toolchain.cmake")
endif()

project(book_manager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libpqxx REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(book_manager main.cpp)

target_link_libraries(book_manager PRIVATE
    libpqxx::pqxx
    nlohmann_json::nlohmann_json
)
