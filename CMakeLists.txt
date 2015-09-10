cmake_minimum_required(VERSION 3.3)
project(LipSync)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(LipSync ${SOURCE_FILES})