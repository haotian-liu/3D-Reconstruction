## Colormap Optimization for 3D Reconstruction

### Dependencies and Libraries

- glm (included in `./Dependencies/glm`)
- tinyply (included in `./Dependencies/tinyply`)
- OpenCV 4
- Eigen 3
- GLEW 2.1.0
- glfw 3.3



### File Structures

- `./Dependencies`: external libraries bundled with the project
- `./assets`: storing model data like `ply` files
- `./shader`: shader files
- `./src`: source code directory, organized in the format of `./src/{ClassName}/{ClassName}.{cpp,h}`
- `./Utility`: helper code directory, organized in the format of `./src/{ClassName}/{ClassName}.{cpp,h}`
- `./output`: screenshot directory



### Compile and Run

Tested on macOS Mojave 10.14.5, with clang and cmake. Configuration in `CMakeLists.txt`.



### Configurations

- `./src/App/App.cpp`

  - L10 (`r->setupPolygon`): model.ply file location

- `./src/ColorMapper/ColorMapper.cpp`

  - L28 (`const std::string path_folder`): directory where keyframes are stored

  - L29 (`const std::string path_file`): camera trajectory log filename, should be stored in the same directory where keyframes are stored, information organized in the form:

    ```
    filename of 1st frame without extension (1 line)
    1st camera matrix (4line, 4x4 matrix)
    _________ (blank line)
    filename of 2nd frame without extension (1 line)
    2nd camera matrix (4line, 4x4 matrix)
    _________ (blank line)
    ...
    
    ...
    filename of last frame without extension (1 line)
    last camera matrix (4line, 4x4 matrix)
    ```

    