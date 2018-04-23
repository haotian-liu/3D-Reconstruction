//
// Created by Haotian on 2017/8/23.
//

#ifndef INC_3DRECONSTRUCTION_APP_H
#define INC_3DRECONSTRUCTION_APP_H

#include <string>
#include "GLInclude.h"
#include "GLFW/glfw3.h"
#include "Renderer/Renderer.h"

class App {
public:
    void init();
    void setTitle(const std::string &title) { windowTitle = title; }
    void setViewport(int width, int height);
    void updateViewport();
    void render();
    void idle(GLFWwindow *window);
    void mouseCallback(GLFWwindow *window, int button, int action, int mods);
    void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void screenShot();

private:
    void monitorFPS(GLFWwindow *window);

    std::string windowTitle;
    Renderer *r;
    int winWidth, winHeight;
};


#endif //INC_3DRECONSTRUCTION_APP_H
