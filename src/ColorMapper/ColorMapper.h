//
// Created by Haotian on 2018/3/22.
//

#ifndef INC_3DRECONSTRUCTION_COLORMAPPER_H
#define INC_3DRECONSTRUCTION_COLORMAPPER_H

#include "Geometry/Shape.h"

class ColorMapper {
public:
    ColorMapper() = default;
    explicit ColorMapper(Shape *shape) : shape(shape) {}
    void switch_to(Shape *shape) { this->shape = shape; }
    bool available() const { return shape != nullptr; }
private:
    Shape *shape = nullptr;
};


#endif //INC_3DRECONSTRUCTION_COLORMAPPER_H
