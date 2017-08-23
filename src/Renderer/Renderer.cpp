//
// Created by Haotian on 2017/8/23.
//

#include "Renderer.h"
#include <fstream>
#include "tinyply/tinyply.h"

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
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), &verts[0], GL_STATIC_DRAW);
    GLuint locVertPos = glGetAttribLocation(shader->ProgramId(), "vertPos");
    glEnableVertexAttribArray(locVertPos);
    glVertexAttribPointer(locVertPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // index
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(GLuint), &faces[0], GL_STATIC_DRAW);
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
    double diffX, diffY;

    if (LBtnDown) {
        diffX = currentX - lastX;
        diffY = currentY - lastY;

        Yaw += diffX;
        Pitch += diffY;

        if (Pitch > 89.f)
            Pitch = 89.f;
        if (Pitch < -89.f)
            Pitch = -89.f;
        updateCamera();
    }

    lastX = currentX;
    lastY = currentY;
}

void Renderer::updateCamera() {
    glm::vec3 front(
            cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
            sin(glm::radians(Pitch)),
            sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))
    );

    viewMatrix = glm::lookAt(
            front * Dist,
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, 1.f, 0.f)
    );
}

void Renderer::render() {
    projMatrix = glm::perspective(glm::radians(60.f), 800.f / 600, 0.001f, 10.f);

    shader->Activate();
    glUniformMatrix4fv(glGetUniformLocation(shader->ProgramId(), "viewMatrix"), 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader->ProgramId(), "modelMatrix"), 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader->ProgramId(), "projMatrix"), 1, GL_FALSE, &projMatrix[0][0]);

    glBindVertexArray(mVao);
    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

    shader->Deactivate();
}

typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;
std::chrono::high_resolution_clock c;

inline std::chrono::time_point<std::chrono::high_resolution_clock> now() {
    return c.now();
}

inline double difference_micros(timepoint start, timepoint end) {
    return (double) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

bool Renderer::loadPolygon() {
    std::ifstream ss(filename, std::ios::binary);
    tinyply::PlyFile file(ss);

    for (auto e : file.get_elements()) {
        std::cout << "element - " << e.name << " (" << e.size << ")" << std::endl;
        for (auto p : e.properties) {
            std::cout << "\tproperty - " << p.name << " (" << tinyply::PropertyTable[p.propertyType].str << ")"
                      << std::endl;
        }
    }
    std::cout << std::endl;

    for (auto c : file.comments) {
        std::cout << "Comment: " << c << std::endl;
    }

    // Define containers to hold the extracted data. The type must match
    // the property type given in the header. Tinyply will interally allocate the
    // the appropriate amount of memory.


    std::vector<GLfloat> verts;
    std::vector<GLfloat> norms;
    std::vector<GLfloat> uvCoords;

    uint32_t vertexCount, normalCount, colorCount, faceCount, faceTexcoordCount, faceColorCount;
    vertexCount = normalCount = colorCount = faceCount = faceTexcoordCount = faceColorCount = 0;

    // The count returns the number of instances of the property group. The vectors
    // above will be resized into a multiple of the property group size as
    // they are "flattened"... i.e. verts = {x, y, z, x, y, z, ...}
    vertexCount = file.request_properties_from_element("vertex", {"x", "y", "z"}, verts);
    normalCount = file.request_properties_from_element("vertex", {"nx", "ny", "nz"}, norms);
    colorCount = file.request_properties_from_element("vertex", {"red", "green", "blue", "alpha"}, colors);

    // For properties that are list types, it is possibly to specify the expected count (ideal if a
    // consumer of this library knows the layout of their format a-priori). Otherwise, tinyply
    // defers allocation of memory until the first instance of the property has been found
    // as implemented in file.read(ss)
    faceCount = file.request_properties_from_element("face", {"vertex_indices"}, faces, 3);
    faceTexcoordCount = file.request_properties_from_element("face", {"texcoord"}, uvCoords, 6);

    // Now populate the vectors...
    timepoint before = now();
    file.read(ss);
    timepoint after = now();

    for (int i=0; i<verts.size(); i+=3) {
        this->verts.emplace_back(verts[i], verts[i + 1], verts[i + 2]);
    }
    for (int i=0; i<norms.size(); i+=3) {
        this->norms.emplace_back(norms[i], norms[i + 1], norms[i + 2]);
    }
    for (int i=0; i<uvCoords.size(); i+=2) {
        this->uvCoords.emplace_back(uvCoords[i], uvCoords[i + 1]);
    }

    // Good place to put a breakpoint!
    std::cout << "Parsing took " << difference_micros(before, after) << "μs: " << std::endl;
    std::cout << "\tRead " << verts.size() << " total vertices (" << vertexCount << " properties)." << std::endl;
    std::cout << "\tRead " << norms.size() << " total normals (" << normalCount << " properties)." << std::endl;
    std::cout << "\tRead " << colors.size() << " total vertex colors (" << colorCount << " properties)." << std::endl;
    std::cout << "\tRead " << faces.size() << " total faces (triangles) (" << faceCount << " properties)." << std::endl;
    std::cout << "\tRead " << uvCoords.size() << " total texcoords (" << faceTexcoordCount << " properties)."
              << std::endl;

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
