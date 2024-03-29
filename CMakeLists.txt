cmake_minimum_required(VERSION 3.8)
project(3DReconstruction)

#set(CMAKE_CXX_COMPILER /usr/local/bin/g++-9)
#set(CMAKE_C_COMPILER /usr/local/bin/gcc-9)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework OpenGL")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/usr/local/Cellar/glew/2.1.0/lib -L/usr/local/Cellar/glfw/3.3/lib -lGLEW -lglfw")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -march=native")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

include_directories(
        /usr/local/include
        Dependencies
        Dependencies/glm/
        src/
        Utility/
)

set(SOURCE_FILES main.cpp
        Dependencies/tinyply/tinyply.cpp src/Renderer/Renderer.cpp src/Renderer/Renderer.h src/App/App.cpp src/App/App.h Utility/Shader/Shader.cpp Utility/Shader/Shader.h Utility/GLInclude.h Utility/Shader/ShaderProgram.cpp Utility/Shader/ShaderProgram.h src/ColorMapper/ColorMapper.cpp src/ColorMapper/ColorMapper.h Dependencies/Geometry/Shape.cpp Dependencies/Geometry/Shape.h Utility/Timer/Timer.h)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(3DReconstruction ${SOURCE_FILES})

target_link_libraries(3DReconstruction ${OpenCV_LIBS})