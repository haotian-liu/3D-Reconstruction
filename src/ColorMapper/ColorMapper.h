//
// Created by Haotian on 2018/3/22.
//

#ifndef INC_3DRECONSTRUCTION_COLORMAPPER_H
#define INC_3DRECONSTRUCTION_COLORMAPPER_H

#include <vector>
#include "opencv/cv.hpp"
#include "Geometry/Shape.h"
#include "Shader/ShaderProgram.h"

struct MapUnit {
    explicit MapUnit(std::string in_path, std::string in_key, const glm::mat4 &transform) :
            path(std::move(in_path)), key(std::move(in_key)), transform(transform) {

    }

    void load_image() {
        auto temp = cv::imread(path + key + ".jpg");
        cv::flip(temp, color_image, 0);
        cv::cvtColor(color_image, grey_image, CV_RGB2GRAY);
//        cv::imshow("window", grey_image);
//        cv::waitKey(0);
    }
    std::string path;
    std::string key;
    glm::mat4 transform;
    std::vector<GLuint> vertices;
    cv::Mat color_image;
    cv::Mat grey_image;
};

class ColorMapper {
public:
    ColorMapper() = default;
    explicit ColorMapper(Shape *shape) : shape(shape) {}
    void switch_to(Shape *shape) { this->shape = shape; }
    void map_color();
    bool available() const { return shape != nullptr; }
private:
    void load_keyframes();
    void load_images();
    void base_map();
    bool compileShader(ShaderProgram *shader, const std::string &vs, const std::string &fs);
    Shape *shape = nullptr;

    std::vector<MapUnit> map_units;
    std::vector<GLfloat> grey_colors;
};


#endif //INC_3DRECONSTRUCTION_COLORMAPPER_H
