cmake_minimum_required(VERSION 3.8)
project(3DReconstruction)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(3DReconstruction ${SOURCE_FILES})