//
// Created by Haotian on 2018/3/22.
//

#ifndef INC_3DRECONSTRUCTION_COLORMAPPER_H
#define INC_3DRECONSTRUCTION_COLORMAPPER_H

#include <vector>
#include "opencv/cv.hpp"
#include "Geometry/Shape.h"

class ColorMapper {
public:
    ColorMapper() = default;
    explicit ColorMapper(Shape *shape) : shape(shape) {}
    void switch_to(Shape *shape) { this->shape = shape; }
    void update_config();
    bool available() const { return shape != nullptr; }
private:
    void load_keyframes();
    void base_map();
    Shape *shape = nullptr;

    std::vector<std::string> keys;
    std::vector<cv::Mat> images_color;
    std::vector<cv::Mat> images_grey;
};


#endif //INC_3DRECONSTRUCTION_COLORMAPPER_H
