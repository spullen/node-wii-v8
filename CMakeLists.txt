cmake_minimum_required(VERSION 3.6)
project(node_wii_v8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(node_wii_v8 ${SOURCE_FILES})