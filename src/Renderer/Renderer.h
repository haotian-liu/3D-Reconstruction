//
// Created by Haotian on 2017/8/23.
//

#ifndef INC_3DRECONSTRUCTION_RENDERER_H
#define INC_3DRECONSTRUCTION_RENDERER_H

#include "GLInclude.h"
#include "GLFW/glfw3.h"
#include <string>
#include <vector>
#include "Shader/ShaderProgram.h"
#include "Geometry/Shape.h"
#include "ColorMapper/ColorMapper.h"

class Renderer {
public:
    Renderer() : shader(new ShaderProgram), modelMatrix(1.f), mVao(0) {
        updateCamera();
    };

    void setupPolygon(const std::string &filename);
    void setupShader(const std::string &vs, const std::string &fs);
    void setupBuffer();
    void render();
    void mouseCallback(GLFWwindow *window, int button, int action, int mods);
    void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void idle(GLFWwindow *window);
    void updateCamera();

private:
    bool compileShader(ShaderProgram *shader, const std::string &vs, const std::string &fs);
    bool loadPolygon();
    bool processPolygon();

    glm::vec3 viewDirection = glm::vec3(1.f, 0.f, 0.f), lightDirection;

    ShaderProgram *shader;
    GLuint mVao, mVbo[5];

    GLfloat Yaw = 90.f, Pitch = 0.f, Dist = 2.f;
    static glm::mat4 viewTransform;

    std::string filename;

    Shape shape;

    glm::mat4 modelMatrix, viewMatrix, projMatrix;
    bool LBtnDown = false, RBtnDown = false;
    ColorMapper cm;
};


#endif //INC_3DRECONSTRUCTION_RENDERER_H
