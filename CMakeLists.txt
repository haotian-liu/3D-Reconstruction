cmake_minimum_required(VERSION 3.8)
project(3DReconstruction)

set(CMAKE_CXX_STANDARD 11)

include_directories(Dependencies)

set(SOURCE_FILES main.cpp
        Dependencies/tinyply/tinyply.cpp src/Renderer/Renderer.cpp src/Renderer/Renderer.h)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(3DReconstruction ${SOURCE_FILES})