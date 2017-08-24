//
// Created by Haotian on 2017/8/23.
//

#include "App.h"

void App::init() {
    r = new Renderer;
    r->setupPolygon("assets/fountain.ply");
    r->setupShader("shader/phong.vert", "shader/phong.frag");
    r->setupBuffer();
}

void App::setViewport(int width, int height) {
    winWidth = width;
    winHeight = height;
    updateViewport();
}

void App::updateViewport() {
    glViewport(0, 0, winWidth, winHeight);
}

void App::render() {
    r->render();
}

void App::idle() {

}

void App::mouseCallback(GLFWwindow *window, int button, int action, int mods) {
    r->mouseCallback(window, button, action, mods);
}

void App::cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    r->cursorPosCallback(window, xpos, ypos);
}

void App::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    r->keyCallback(window, key, scancode, action, mods);
}
