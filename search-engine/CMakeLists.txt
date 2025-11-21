cmake_minimum_required(VERSION 3.16)
project(search_engine C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

file(GLOB SOURCES "src/*.c")

add_executable(search_engine ${SOURCES})

enable_testing()
add_subdirectory(tests)
