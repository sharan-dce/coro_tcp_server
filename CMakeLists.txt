cmake_minimum_required(VERSION 3.30)

project(co_net LANGUAGES CXX)

set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 23)

add_subdirectory(tcp)
add_subdirectory(coro)

add_executable(echo_server main.cpp)
target_link_libraries(echo_server tcp)
