//
// Created by Haotian on 2017/8/23.
//

#include "App.h"
#include <chrono>

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
    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    r->render();
}

void App::idle(GLFWwindow *window) {
    r->idle(window);
    monitorFPS(window);
}

void App::monitorFPS(GLFWwindow *window) {
    static auto last = std::chrono::high_resolution_clock::now();
    static auto lastRefresh = last;
    auto current = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> >>
            (current - last).count();
    auto refreshDuration = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> >>
            (current - lastRefresh).count();
    last = current;
    if (duration < 0.0001) { return; }
    if (refreshDuration < 0.1) { return; }
    lastRefresh = current;
    GLuint FPS = static_cast<GLuint>(1.0 / duration);
    std::string newTitle = windowTitle + " (" + std::to_string(FPS) +  " fps)";
    glfwSetWindowTitle(window, newTitle.c_str());
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

void App::screenShot() {
    r->screenShot();
}
