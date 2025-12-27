cmake_minimum_required(VERSION 3.16)
project(search_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "src/*.cpp")

add_executable(search_engine ${SOURCES})

include_directories(include)
include_directories(vendor/spdlog/include)
include_directories(vendor/libstemmer/include)

add_subdirectory(vendor/libstemmer)

# setup mongocxx
find_package(PkgConfig REQUIRED)
pkg_check_modules(MONGOCXX REQUIRED libmongocxx)
target_include_directories(search_engine PRIVATE ${MONGOCXX_INCLUDE_DIRS})
target_link_libraries(search_engine PRIVATE ${MONGOCXX_LIBRARIES})
target_compile_options(search_engine PRIVATE ${MONGOCXX_CFLAGS_OTHER})

# libstemmer
target_link_libraries(search_engine PRIVATE stemmer)

enable_testing()

add_subdirectory(tests)