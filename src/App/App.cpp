//
// Created by Haotian on 2017/8/23.
//

#include "App.h"

void App::init() {
    r = new Renderer;
    r->setName("assets/bunny.ply");
    r->samplePly();
}

void App::render() {
}
