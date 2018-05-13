//
// Created by Haotian on 2018/3/22.
//

#ifndef INC_3DRECONSTRUCTION_COLORMAPPER_H
#define INC_3DRECONSTRUCTION_COLORMAPPER_H

#include <vector>
#include "opencv/cv.hpp"
#include "Geometry/Shape.h"
#include "Shader/ShaderProgram.h"

#define RECONSTRUCTION_max(a, b) ((a) > (b) ? (a) : (b))

struct MapUnit {
    explicit MapUnit(std::string in_path, std::string in_key, const glm::mat4 &transform) :
            path(std::move(in_path)), key(std::move(in_key)), transform(transform) {
        for (int i=0; i<21; i++) {
            for (int j=0; j<17; j++) {
                control_vertices[i][j] = glm::vec2(0.f);
            }
        }
    }

    void load_image() {
        auto temp = cv::imread(path + key + ".jpg");
        temp.convertTo(color_image, CV_32F, 1.f / 255, 0);
        cv::cvtColor(color_image, grey_image, CV_BGR2GRAY);
        cv::Scharr(grey_image, grad_x, -1, 0, 1);
        cv::Scharr(grey_image, grad_y, -1, 1, 0);
        cv::normalize(grad_x, grad_x, 0, 1, cv::NORM_MINMAX);
        cv::normalize(grad_y, grad_y, 0, 1, cv::NORM_MINMAX);
    }
    std::string path;
    std::string key;
    glm::mat4 transform;
    std::vector<GLuint> vertices;
    cv::Mat color_image;
    cv::Mat grey_image;
    cv::Mat grad_x, grad_y;

    glm::vec2 control_vertices[21][17];
};
struct GLUnit {
    const int SSAA = 2;
    const int GLWidth = 1280, GLHeight = 1024;
    const int frameWidth = GLWidth * SSAA, frameHeight = GLHeight * SSAA;
    const glm::vec2 f = glm::vec2(1050.f, 1050.f), c = glm::vec2(639.5f, 511.5f);

    GLuint fbo, rbo, vao, vbo[2];
    ShaderProgram shader;
};

class ColorMapper {
public:
    ColorMapper() = default;
    explicit ColorMapper(Shape *shape) : shape(shape) {}
    void switch_to(Shape *shape) { this->shape = shape; }
    void map_color();
    bool available() const { return shape != nullptr; }
private:
    void load_keyframes();
    void load_images();
    void base_map();

    void prepare_OGL(GLUnit &u);
    void destroy_OGL(GLUnit &u);
    void register_views(GLUnit &u);
    void register_depths(GLUnit &u);
    void register_vertices(GLUnit &u);
    void color_vertices(GLUnit &u, bool need_color);
    void optimize_pose(GLUnit &u);
    bool compileShader(ShaderProgram *shader, const std::string &vs, const std::string &fs);
    glm::vec2 bilerp(const glm::vec2 &v1, const glm::vec2 &v2, const glm::vec2 &v3, const glm::vec2 &v4, float cx,
                     float cy) const;
    glm::mat2 bilerp_gradient(const glm::vec2 &v1, const glm::vec2 &v2, const glm::vec2 &v3, const glm::vec2 &v4,
                              float cx, float cy) const;
    float getColorSubpix(const cv::Mat& img, cv::Point2f pt);
    inline bool within_depth(GLuint vert_id, float pixel, float z) { return std::fabs(z - pixel) < RECONSTRUCTION_max(0.001, best_depths[vert_id] * 5); }

    Shape *shape = nullptr;

    int iteration;

    std::vector<MapUnit> map_units;
    std::vector<GLfloat> grey_colors;
    std::vector<float> best_views;
    std::vector<float> best_depths;
};


#endif //INC_3DRECONSTRUCTION_COLORMAPPER_H
