//
// Created by Haotian on 2017/8/23.
//

#include "Renderer.h"

glm::mat4 Renderer::viewTransform(1.f);

void Renderer::setupPolygon(const std::string &filename) {
    this->filename = filename;
    loadPolygon();
}

void Renderer::setupShader(const std::string &vs, const std::string &fs) {
    compileShader(shader, vs, fs);
}

void Renderer::setupBuffer() {
    // Create the buffers for the vertices atttributes
    glGenVertexArrays(1, &mVao);
    glBindVertexArray(mVao);
    glGenBuffers(sizeof(mVbo) / sizeof(GLuint), mVbo);
    // vertex coordinate
    glBindBuffer(GL_ARRAY_BUFFER, mVbo[0]);
    glBufferData(GL_ARRAY_BUFFER, shape.vertices.size() * sizeof(glm::vec3), &shape.vertices[0], GL_STATIC_DRAW);
    GLuint locVertPos = (GLuint)glGetAttribLocation(shader->ProgramId(), "vertPos");
    glEnableVertexAttribArray(locVertPos);
    glVertexAttribPointer(locVertPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // vertex normal
    glBindBuffer(GL_ARRAY_BUFFER, mVbo[1]);
    glBufferData(GL_ARRAY_BUFFER, shape.normals.size() * sizeof(glm::vec3), &shape.normals[0], GL_STATIC_DRAW);
    GLuint locVertNormal = (GLuint)glGetAttribLocation(shader->ProgramId(), "vertNormal");
    glEnableVertexAttribArray(locVertNormal);
    glVertexAttribPointer(locVertNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // vertex color
    glBindBuffer(GL_ARRAY_BUFFER, mVbo[2]);
    glBufferData(GL_ARRAY_BUFFER, shape.colors.size() * sizeof(glm::vec4), &shape.colors[0], GL_STATIC_DRAW);
    GLuint locVertColor = (GLuint)glGetAttribLocation(shader->ProgramId(), "vertColor");
    glEnableVertexAttribArray(locVertColor);
    glVertexAttribPointer(locVertColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
    // index
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVbo[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, shape.faces.size() * sizeof(GLuint), &shape.faces[0], GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void Renderer::mouseCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (GLFW_PRESS == action) {
            LBtnDown = true;
        } else if (GLFW_RELEASE == action) {
            LBtnDown = false;
        }
    }
}

void Renderer::cursorPosCallback(GLFWwindow *window, double currentX, double currentY) {
    static double lastX, lastY;
    GLfloat diffX, diffY;

    diffX = currentX - lastX;
    diffY = currentY - lastY;

    if (LBtnDown) {

//        Yaw += diffX;
//        Pitch += diffY;

        viewTransform =
                glm::rotate(glm::mat4(1.f), glm::radians(diffX), glm::vec3(viewTransform * glm::vec4(0.f, 1.f, 0.f, 1.f))) *
                glm::rotate(glm::mat4(1.f), glm::radians(diffY), glm::vec3(viewTransform * glm::vec4(1.f, 0.f, 0.f, 1.f))) *
                viewTransform;
        viewDirection = glm::mat3(viewTransform) * glm::vec3(0.f, 0.f, 1.f);

//        viewDirection = glm::rotate(viewDirection, glm::radians(diffX), glm::vec3(1.f, 0.f, 0.f));
//        viewDirection = glm::rotate(viewDirection, glm::radians(diffY), glm::vec3(0.f, 1.f, 0.f));

        updateCamera();
    }

    lastX = currentX;
    lastY = currentY;
}

void Renderer::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
//    if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
//        viewDirection = glm::rotate(viewDirection, glm::radians(2.f), glm::vec3(0.f, 0.f, 1.f));
//    }
//    if (key == GLFW_KEY_D && action != GLFW_RELEASE) {
//        viewDirection = glm::rotate(viewDirection, glm::radians(-2.f), glm::vec3(0.f, 0.f, 1.f));
//    }
    updateCamera();
}

void Renderer::idle(GLFWwindow *window) {
    if (cm.available()) {
        //
    }
}

void Renderer::updateCamera() {
//    viewDirection = glm::vec3(
//            sin(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
//            sin(glm::radians(Yaw)) * sin(glm::radians(Pitch)),
//            cos(glm::radians(Yaw))
//    );

    //lightDirection = normalize(glm::vec3(100.f, 200.f, 100.f));
    viewDirection = glm::normalize(viewDirection);
    lightDirection = glm::mat3(viewTransform) * viewDirection;

    viewMatrix = glm::lookAt(
            viewDirection * Dist,
            glm::vec3(0.f, 0.f, 0.f),
            glm::mat3(viewTransform) * glm::vec3(0.f, 1.f, 0.f)
    );
}

void Renderer::render() {
    projMatrix = glm::perspective(glm::radians(60.f), 800.f / 600, 0.001f, 10.f);
    modelMatrix = glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f))
                  * glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f, 0.f, 1.f))
                  * glm::translate(glm::mat4(1.f), shape.offset);

    shader->Activate();
    glUniformMatrix4fv(glGetUniformLocation(shader->ProgramId(), "viewMatrix"), 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader->ProgramId(), "modelMatrix"), 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader->ProgramId(), "projMatrix"), 1, GL_FALSE, &projMatrix[0][0]);

    glUniform3fv(glGetUniformLocation(shader->ProgramId(), "LightDirection"), 1, &lightDirection[0]);

    glBindVertexArray(mVao);
    glDrawElements(GL_TRIANGLES, shape.faces.size(), GL_UNSIGNED_INT, 0);

    shader->Deactivate();
}

bool Renderer::loadPolygon() {
    shape.load_from_ply(filename);
    cm.switch_to(&shape);
    return processPolygon();
}

bool Renderer::processPolygon() {
    shape.centralize();
    if (shape.normals.empty()) { shape.generate_normals(); }
    if (shape.colors.empty()) { shape.initialize_colors(); }
    return true;
}

bool Renderer::compileShader(ShaderProgram *shader, const std::string &vs, const std::string &fs) {
    auto VertexShader = new Shader(Shader::Shader_Vertex);
    if (!VertexShader->CompileSourceFile(vs)) {
        delete VertexShader;
        return false;
    }

    auto FragmentShader = new Shader(Shader::Shader_Fragment);
    if (!FragmentShader->CompileSourceFile(fs)) {
        delete VertexShader;
        delete FragmentShader;
        return false;
    }
    shader->AddShader(VertexShader, true);
    shader->AddShader(FragmentShader, true);
    //Link the program.
    return shader->Link();
}
