cmake_minimum_required(VERSION 3.8)
project(3DReconstruction)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework OpenGL -lGLEW -lglfw")

include_directories(Dependencies
        src/
        Utility/
        Dependencies/glm/
        /usr/local/Cellar/glew/2.0.0/include/
        /usr/local/Cellar/glfw/3.2.1/include/
        /opt/local/include/SOIL/
        )

link_directories(
        /usr/local/Cellar/glew/2.0.0/lib/
        /usr/local/Cellar/glfw/3.2.1/lib/
)

set(SOURCE_FILES main.cpp
        Dependencies/tinyply/tinyply.cpp src/Renderer/Renderer.cpp src/Renderer/Renderer.h src/App/App.cpp src/App/App.h Utility/Shader/Shader.cpp Utility/Shader/Shader.h Utility/GLInclude.h Utility/Shader/ShaderProgram.cpp Utility/Shader/ShaderProgram.h src/ColorMapper/ColorMapper.cpp src/ColorMapper/ColorMapper.h Dependencies/Geometry/Shape.cpp Dependencies/Geometry/Shape.h)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(3DReconstruction ${SOURCE_FILES})

target_link_libraries(3DReconstruction /opt/local/lib/libSOIL.dylib)