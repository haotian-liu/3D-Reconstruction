cmake_minimum_required(VERSION 3.8)
project(3DReconstruction)

set(CMAKE_CXX_COMPILER /usr/local/bin/g++-7)
set(CMAKE_C_COMPILER /usr/local/bin/gcc-7)

set(CMAKE_CXX_STANDARD 11)

set(OPENCV_LINK_FLAGS -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_photo -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dpm -lopencv_face -lopencv_fuzzy -lopencv_img_hash -lopencv_line_descriptor -lopencv_optflow -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_text -lopencv_dnn -lopencv_plot -lopencv_xfeatures2d -lopencv_shape -lopencv_video -lopencv_ml -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_flann -lopencv_xobjdetect -lopencv_imgcodecs -lopencv_objdetect -lopencv_xphoto -lopencv_imgproc -lopencv_core)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework OpenGL -lGLEW -lglfw")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

include_directories(Dependencies
        src/
        Utility/
        Dependencies/glm/
        /usr/local/Cellar/glew/2.0.0/include/
        /usr/local/Cellar/glfw/3.2.1/include/
        /usr/local/Cellar/opencv/3.4.1_2/include/
        )

link_directories(
        /usr/local/Cellar/glew/2.0.0/lib/
        /usr/local/Cellar/glfw/3.2.1/lib/
        /usr/local/Cellar/opencv/3.4.1_2/include/
)

set(SOURCE_FILES main.cpp
        Dependencies/tinyply/tinyply.cpp src/Renderer/Renderer.cpp src/Renderer/Renderer.h src/App/App.cpp src/App/App.h Utility/Shader/Shader.cpp Utility/Shader/Shader.h Utility/GLInclude.h Utility/Shader/ShaderProgram.cpp Utility/Shader/ShaderProgram.h src/ColorMapper/ColorMapper.cpp src/ColorMapper/ColorMapper.h Dependencies/Geometry/Shape.cpp Dependencies/Geometry/Shape.h)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(3DReconstruction ${SOURCE_FILES})

target_link_libraries(3DReconstruction ${OPENCV_LINK_FLAGS})