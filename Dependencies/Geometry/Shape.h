//
// Created by Haotian on 2018/3/22.
//

#ifndef INC_3DRECONSTRUCTION_SHAPE_H
#define INC_3DRECONSTRUCTION_SHAPE_H

#include "GLInclude.h"
#include <utility>
#include <vector>
#include <string>

struct Shape {
    Shape() {}
    Shape(std::vector<glm::vec3> in_vertices, std::vector<glm::vec3> in_normals, std::vector<glm::vec2> in_uvs,
          std::vector<GLuint> in_faces, std::vector<glm::vec4> in_colors) : vertices(std::move(in_vertices)),
                                                                            normals(std::move(in_normals)),
                                                                            uvs(std::move(in_uvs)),
                                                                            faces(std::move(in_faces)),
                                                                            colors(std::move(in_colors)) {
    }

    bool load_from_ply(const std::string &filename);
    void export_to_ply(const std::string &filename);

    std::vector<glm::vec3> vertices, normals;
    std::vector<glm::vec2> uvs;

    std::vector<GLuint> faces;
    std::vector<glm::vec4> colors;
    glm::mat4 transform = glm::mat4(1.f);

    glm::vec3 offset = glm::vec3(0.f);

    static void convert_to_glm(const std::vector<GLfloat> &in_vector, std::vector<glm::vec2> &out_glm_vector);
    static void convert_to_glm(const std::vector<GLfloat> &in_vector, std::vector<glm::vec3> &out_glm_vector);
    static void convert_to_glm(const std::vector<GLfloat> &in_vector, std::vector<glm::vec4> &out_glm_vector);

    void centralize();
    void generate_normals();
    void initialize_colors();
};


#endif //INC_3DRECONSTRUCTION_SHAPE_H
