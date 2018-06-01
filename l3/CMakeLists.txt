cmake_minimum_required(VERSION 3.1)
project(pct_3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lpthread")

set(SOURCE_FILES main.cpp)
add_executable(pct_3 ${SOURCE_FILES})