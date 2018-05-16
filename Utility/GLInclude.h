//
// Created by Haotian on 2017/8/23.
//

#ifndef INC_3DRECONSTRUCTION_GLINCLUDE_H
#define INC_3DRECONSTRUCTION_GLINCLUDE_H

#include <GL/glew.h>
#include "ext.hpp"
#define DEBUG
#ifdef DEBUG
#include <iostream>
#endif

inline std::ostream& operator << (std::ostream& stream, const glm::mat4 &matrix) {
    for (int i=0; i<4; i++) {
        for (int j=0; j<4; j++) {
            stream << matrix[i][j] << " ";
        }
        stream << '\n';
    }
    return stream;
}

#endif //INC_3DRECONSTRUCTION_GLINCLUDE_H
