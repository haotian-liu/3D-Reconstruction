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
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), &verts[0], GL_STATIC_DRAW);
    GLuint locVertPos = (GLuint)glGetAttribLocation(shader->ProgramId(), "vertPos");
    glEnableVertexAttribArray(locVertPos);
    glVertexAttribPointer(locVertPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // vertex normal
    glBindBuffer(GL_ARRAY_BUFFER, mVbo[1]);
    glBufferData(GL_ARRAY_BUFFER, norms.size() * sizeof(GLfloat), &norms[0], GL_STATIC_DRAW);
    GLuint locVertNormal = (GLuint)glGetAttribLocation(shader->ProgramId(), "vertNormal");
    glEnableVertexAttribArray(locVertNormal);
    glVertexAttribPointer(locVertNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // index
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVbo[2]);
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

        updateCamera();
    }

    lastX = currentX;
    lastY = currentY;
}

void Renderer::updateCamera() {
    viewDirection = glm::vec3(
            sin(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
            sin(glm::radians(Yaw)) * sin(glm::radians(Pitch)),
            cos(glm::radians(Yaw))
    );

    //lightDirection = normalize(glm::vec3(100.f, 200.f, 100.f));
    viewDirection = glm::normalize(viewDirection);
    lightDirection = viewDirection;
    halfVector = glm::normalize(lightDirection + viewDirection);

    viewMatrix = glm::lookAt(
            viewDirection * Dist,
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

    glUniform3fv(glGetUniformLocation(shader->ProgramId(), "LightDirection"), 1, &lightDirection[0]);
    glUniform3fv(glGetUniformLocation(shader->ProgramId(), "HalfVector"), 1, &halfVector[0]);

    glBindVertexArray(mVao);
    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

    shader->Deactivate();
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
    file.read(ss);

    processPolygon();

    // Good place to put a breakpoint!
    std::cout << "\tRead " << verts.size() << " total vertices (" << vertexCount << " properties)." << std::endl;
    std::cout << "\tRead " << norms.size() << " total normals (" << normalCount << " properties)." << std::endl;
    std::cout << "\tRead " << colors.size() << " total vertex colors (" << colorCount << " properties)." << std::endl;
    std::cout << "\tRead " << faces.size() << " total faces (triangles) (" << faceCount << " properties)." << std::endl;
    std::cout << "\tRead " << uvCoords.size() << " total texcoords (" << faceTexcoordCount << " properties)."
              << std::endl;

    return true;
}

bool Renderer::processPolygon() {
    if (norms.size() == 0) {
        generateNormals();
    }
    return true;
}

glm::vec3 Renderer::getVertVector(int index) {
    return glm::vec3(verts[index], verts[index + 1], verts[index + 2]);
}

bool Renderer::updateNormal(int index, const glm::vec3 &Normal) {
    norms[index] = Normal.x;
    norms[index + 1] = Normal.y;
    norms[index + 2] = Normal.z;
    return true;
}

bool Renderer::generateNormals() {
    norms.resize(verts.size());
    for (int i = 0; i < faces.size(); i += 3) {
        glm::vec3 p1 = getVertVector(faces[i] * 3);
        glm::vec3 p2 = getVertVector(faces[i + 1] * 3);
        glm::vec3 p3 = getVertVector(faces[i + 2] * 3);

        glm::vec3 u = p2 - p1, v = p3 - p1;
        glm::vec3 Normal;
        Normal.x = u.y * v.z - u.z * v.y;
        Normal.y = u.z * v.x - u.x * v.z;
        Normal.z = u.x * v.y - u.y * v.x;

        updateNormal(faces[i] * 3, Normal);
        updateNormal(faces[i + 1] * 3, Normal);
        updateNormal(faces[i + 2] * 3, Normal);
    }
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
