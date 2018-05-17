//
// Created by Haotian on 2017/8/23.
//

#include "Renderer.h"
#include <boost/lexical_cast.hpp>
#include <sys/stat.h>
#include <fstream>

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
        viewTransform =
                glm::rotate(glm::mat4(1.f), glm::radians(-diffX), glm::vec3(viewTransform * glm::vec4(0.f, 1.f, 0.f, 1.f))) *
                glm::rotate(glm::mat4(1.f), glm::radians(-diffY), glm::vec3(viewTransform * glm::vec4(1.f, 0.f, 0.f, 1.f))) *
                viewTransform;
        updateCamera();
    }

    lastX = currentX;
    lastY = currentY;
}

void Renderer::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    static cv::Mat flipped;
    static auto start_timestamp = std::time(0);
    static auto folder = boost::lexical_cast<std::string>(start_timestamp);
    static auto dir = "output/" + folder;
    static bool dirCreated = false;

    if (action != GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_UP: centerPoint.y += 0.01; break;
            case GLFW_KEY_DOWN: centerPoint.y -= 0.01; break;
            case GLFW_KEY_LEFT: centerPoint.x -= 0.01; break;
            case GLFW_KEY_RIGHT: centerPoint.x += 0.01; break;

            case GLFW_KEY_I: Dist /= 1.01; break;
            case GLFW_KEY_J: Dist *= 1.01; break;
        }
    }

    if (key == GLFW_KEY_S && action != GLFW_RELEASE) {
        if (!dirCreated) {
            dirCreated = true;
            mkdir(dir.c_str(), 0777);
        }
        auto timestamp = std::time(0) - start_timestamp;
        auto ts = boost::lexical_cast<std::string>(timestamp);
        printf("Snapshot taken!\n");
        cv::flip(screenshot_data, flipped, 0);
//        cv::resize(flipped, screenshot_data, cv::Size(640, 480), 0, 0, cv::INTER_CUBIC);
//        cv::imwrite("output/" + folder + "/" + ts + ".jpg", flipped);

        std::ofstream out;
        out.open("output/" + folder + "/pose.log", std::ios::app);
        out << timestamp << std::endl;
        for (int i=0; i<4; i++) {
            for (int j=0; j<4; j++) {
                out << viewMatrix[j][i] << " ";
            }
            out << std::endl;
        }
        out << std::endl;
        out.close();
    }
    updateCamera();
}

void Renderer::screenShot() {
    glReadPixels(0, 0, 640 * 2, 480 * 2, GL_BGR, GL_UNSIGNED_BYTE, screenshot_raw);
    screenshot_data = cv::Mat(480 * 2, 640 * 2, CV_8UC3, screenshot_raw);
}

void Renderer::idle(GLFWwindow *window) {
    if (cm.available()) {
        //
    }
}

void Renderer::updateCamera() {
    viewDirection = glm::mat3(viewTransform) * glm::vec3(0.f, 0.f, 1.f);
    viewDirection = glm::normalize(viewDirection);
    lightDirection = viewDirection - shape.offset;

    viewMatrix = glm::lookAt(
            viewDirection * Dist - shape.offset,
//            glm::vec3(0.f, 0.f, 0.f),
            centerPoint - shape.offset,
            glm::mat3(viewTransform) * glm::vec3(0.f, 1.f, 0.f)
    );
}

void Renderer::render() {
    projMatrix = glm::perspective(glm::radians(60.f), 640.f / 480, 0.001f, 10.f);
//    modelMatrix = glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f))
//                  * glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f, 0.f, 1.f))
//                  * glm::translate(glm::mat4(1.f), shape.offset);
    modelMatrix = glm::mat4(1.f);
    updateCamera();

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
    if (shape.colors.empty()) {
        shape.initialize_colors();
        cm.map_color();
    }
    shape.export_to_ply("output/recon.ply");
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
