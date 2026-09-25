cmake_minimum_required(VERSION 3.15)

# --- Conan 1: подключаем toolchain и ищем CONFIG-пакеты в build/ ---
if(NOT CMAKE_TOOLCHAIN_FILE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/conan_toolchain.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/build/conan_toolchain.cmake"
            CACHE FILEPATH "Conan toolchain" FORCE)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conan_toolchain.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/conan_toolchain.cmake"
            CACHE FILEPATH "Conan toolchain" FORCE)
    endif()
endif()

# Чтобы find_package(CONFIG) нашёл сгенерированные Conan конфиги в build/
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

project(book_manager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libpqxx CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(book_manager main.cpp)

target_link_libraries(book_manager PRIVATE
    libpqxx::pqxx
    nlohmann_json::nlohmann_json
)
