#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "App/App.h"

const int WinWidth = 800, WinHeight = 600;

auto app = new App;

inline static void mouseCallback(GLFWwindow *window, int button, int action, int mods) {
    app->mouseCallback(window, button, action, mods);
}
inline static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    app->keyCallback(window, key, scancode, action, mods);
}
inline static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    app->cursorPosCallback(window, xpos, ypos);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(WinWidth, WinHeight, "Learn GL", nullptr, nullptr);

    int screenWidth, screenHeight;

    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

    if (nullptr == window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;

    if (GLEW_OK != glewInit()) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
    }

    app->setViewport(screenWidth, screenHeight);
    app->init();

    glfwSetMouseButtonCallback(window, mouseCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.5f, 0.5f, 0.5f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        app->render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
