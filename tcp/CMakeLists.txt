cmake_minimum_required(VERSION 3.30)

add_library(tcp STATIC 
    resolver.cpp
    tcp.cpp
)

target_include_directories(tcp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
