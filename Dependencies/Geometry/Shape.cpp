//
// Created by Haotian on 2018/3/22.
//

#include "Shape.h"
#include <fstream>
#include "tinyply/tinyply.h"

bool Shape::load_from_ply(const std::string &filename) {
    std::vector<GLfloat> verts;
    std::vector<GLfloat> norms;
    std::vector<GLfloat> colors;
    std::vector<GLubyte> colors_byte;

    std::vector<GLfloat> uvCoords;


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

    for (auto &c : file.comments) {
        std::cout << "Comment: " << c << std::endl;
    }

    // Define containers to hold the extracted data. The type must match
    // the property type given in the header. Tinyply will interally allocate the
    // the appropriate amount of memory.

    size_t vertexCount, normalCount, colorCount, faceCount, faceTexcoordCount;

    // The count returns the number of instances of the property group. The vectors
    // above will be resized into a multiple of the property group size as
    // they are "flattened"... i.e. verts = {x, y, z, x, y, z, ...}
    vertexCount = file.request_properties_from_element("vertex", {"x", "y", "z"}, verts);
    normalCount = file.request_properties_from_element("vertex", {"nx", "ny", "nz"}, norms);
    colorCount = file.request_properties_from_element("vertex", {"red", "green", "blue", "alpha"}, colors_byte);

    // For properties that are list types, it is possibly to specify the expected count (ideal if a
    // consumer of this library knows the layout of their format a-priori). Otherwise, tinyply
    // defers allocation of memory until the first instance of the property has been found
    // as implemented in file.read(ss)
    faceCount = file.request_properties_from_element("face", {"vertex_indices"}, faces, 3);
    faceTexcoordCount = file.request_properties_from_element("face", {"texcoord"}, uvCoords, 6);

    // Now populate the vectors...
    file.read(ss);

    for (const auto &c : colors_byte) {
        colors.push_back(static_cast<GLfloat>(c) / 255.f);
    }

    convert_to_glm(verts, vertices);
    convert_to_glm(norms, normals);
    convert_to_glm(colors, this->colors);
    convert_to_glm(uvCoords, uvs);

    // Good place to put a breakpoint!
    std::cout << "\tRead " << verts.size() << " total vertices (" << vertexCount << " properties)." << std::endl;
    std::cout << "\tRead " << norms.size() << " total normals (" << normalCount << " properties)." << std::endl;
    std::cout << "\tRead " << colors.size() << " total vertex colors (" << colorCount << " properties)." << std::endl;
    std::cout << "\tRead " << faces.size() << " total faces (triangles) (" << faceCount << " properties)." << std::endl;
    std::cout << "\tRead " << uvCoords.size() << " total texcoords (" << faceTexcoordCount << " properties)."
              << std::endl;

    return true;
}

void Shape::centralize() {
    glm::vec3 Max, Min;
    if (vertices.empty()) { return; }
    Max = Min = vertices[0];
    for (const auto &v : vertices) {
        if (Max.x < v.x) { Max.x = v.x; }
        if (Max.y < v.y) { Max.y = v.y; }
        if (Max.z < v.z) { Max.z = v.z; }
        if (Min.x > v.x) { Min.x = v.x; }
        if (Min.y > v.y) { Min.y = v.y; }
        if (Min.z > v.z) { Min.z = v.z; }
    }
    offset = -(Max + Min) / 2.f;
}

void Shape::generate_normals() {
    normals.resize(vertices.size());

    for (int i = 0; i < faces.size(); i += 3) {
        GLuint f1 = faces[i], f2 = faces[i + 1], f3 = faces[i + 2];
        glm::vec3 p1 = vertices[f1];
        glm::vec3 p2 = vertices[f2];
        glm::vec3 p3 = vertices[f3];

        glm::vec3 u = p2 - p1, v = p3 - p1;
        glm::vec3 Normal;
        Normal.x = u.y * v.z - u.z * v.y;
        Normal.y = u.z * v.x - u.x * v.z;
        Normal.z = u.x * v.y - u.y * v.x;

        Normal = glm::normalize(Normal);

        normals[f1] = normals[f2] = normals[f3] = Normal;
    }
}

void Shape::initialize_colors() {
    colors.resize(vertices.size(), glm::vec4(0.5f));
}

void Shape::convert_to_glm(const std::vector<GLfloat> &in_vector, std::vector<glm::vec2> &out_glm_vector) {
    for (int i=0; i<in_vector.size(); i+=2) {
        auto head = in_vector.begin() + i;
        out_glm_vector.emplace_back(*(head), *(head + 1));
    }
}

void Shape::convert_to_glm(const std::vector<GLfloat> &in_vector, std::vector<glm::vec3> &out_glm_vector) {

    for (int i=0; i<in_vector.size(); i+=3) {
        auto head = in_vector.begin() + i;
        out_glm_vector.emplace_back(*(head), *(head + 1), *(head + 2));
    }
}

void Shape::convert_to_glm(const std::vector<GLfloat> &in_vector, std::vector<glm::vec4> &out_glm_vector) {
    for (int i=0; i<in_vector.size(); i+=4) {
        auto head = in_vector.begin() + i;
        out_glm_vector.emplace_back(*(head), *(head + 1), *(head + 2), *(head + 3));
    }
}

void Shape::export_to_ply(const std::string &filename) {
    std::filebuf fb;
    fb.open(filename, std::ios::out | std::ios::binary);
    std::ostream outputStream(&fb);

    tinyply::PlyFile exampleOutFile;

    std::vector<float> verts, normals;
    std::vector<GLubyte> colors;

    for (int i=0; i<vertices.size(); i++) {
        verts.push_back(this->vertices[i].x);
        verts.push_back(this->vertices[i].y);
        verts.push_back(this->vertices[i].z);
        normals.push_back(this->normals[i].x);
        normals.push_back(this->normals[i].y);
        normals.push_back(this->normals[i].z);
        colors.push_back(255 * this->colors[i].x);
        colors.push_back(255 * this->colors[i].y);
        colors.push_back(255 * this->colors[i].z);
        colors.push_back(255 * this->colors[i].w);
    }

    exampleOutFile.add_properties_to_element("vertex", { "x", "y", "z" }, verts);
    exampleOutFile.add_properties_to_element("vertex", { "nx", "ny", "nz" }, normals);
    exampleOutFile.add_properties_to_element("vertex", { "red", "green", "blue", "alpha" }, colors);

    exampleOutFile.add_properties_to_element("face", { "vertex_indices" }, faces, 3, tinyply::PlyProperty::Type::UINT8);

    exampleOutFile.write(outputStream, false);

    fb.close();
}
