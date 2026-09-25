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

project(bookypedia CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(Boost CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(libbookypedia STATIC
	src/menu/menu.cpp
	src/menu/menu.h
	src/ui/view.cpp
	src/ui/view.h
	src/app/use_cases.h
	src/app/use_cases_impl.cpp
	src/app/use_cases_impl.h
	src/domain/author.cpp
	src/domain/author.h
	src/domain/author_fwd.h
	src/domain/book.h
	src/domain/book_fwd.h
	src/util/tagged.h
	src/util/tagged_uuid.cpp
	src/util/tagged_uuid.h
	src/postgres/postgres.cpp
	src/postgres/postgres.h
)
target_link_libraries(libbookypedia PUBLIC
    Boost::boost
    Threads::Threads
    libpqxx::pqxx
)

add_executable(bookypedia
	src/bookypedia.cpp
	src/bookypedia.h
	src/main.cpp
)
target_link_libraries(bookypedia PRIVATE Boost::boost libbookypedia)

add_executable(tests
	tests/use_case_tests.cpp
	tests/tagged_uuid_tests.cpp
)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain GTest::gtest libbookypedia)
