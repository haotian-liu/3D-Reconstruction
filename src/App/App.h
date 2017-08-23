//
// Created by Haotian on 2017/8/23.
//

#ifndef INC_3DRECONSTRUCTION_APP_H
#define INC_3DRECONSTRUCTION_APP_H

#include <GL/glew.h>
#include "Renderer/Renderer.h"

class App {
public:
    void init();
    void render();

private:
    Renderer *r;
};


#endif //INC_3DRECONSTRUCTION_APP_H
