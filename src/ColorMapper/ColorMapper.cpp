//
// Created by Haotian on 2018/3/22.
//

#include "ColorMapper.h"
#include <fstream>

void ColorMapper::map_color() {
    load_keyframes();
    load_images();
    base_map();
}

void ColorMapper::load_keyframes() {
    map_units.clear();

    const std::string path_folder = "output/1524498778/";
    const std::string path_file = "pose.log";
    std::fstream keyFrameFile(path_folder + path_file);

    std::string imageId;
    glm::mat4 transform;
    while (!keyFrameFile.eof()) {
        keyFrameFile >> imageId;
        for (int i=0; i<4; i++) {
            for (int j=0; j<4; j++) {
                keyFrameFile >> transform[j][i];
            }
        }
        map_units.emplace_back(path_folder, imageId, transform);
    }
}

void ColorMapper::load_images() {
    for (auto &mapper : map_units) {
        mapper.load_image();
    }
}

void ColorMapper::base_map() {
    const glm::mat4 projMatrix = glm::perspective(glm::radians(60.f), 640.f / 480, 0.001f, 10.f);
    glm::mat4 MVPMatrix;
    GLfloat z_buffer[640][480];
    for (const auto &mapper : map_units) {
        MVPMatrix = projMatrix * mapper.transform;
        glm::vec4 vert;
        int cx, cy;
        for (int i=0; i<640; i++) {
            for (int j=0; j<480; j++) {
                z_buffer[i][j] = -10.f;
            }
        }

        for (int i=0; i<shape->vertices.size(); i++) {
            vert = MVPMatrix * glm::vec4(shape->vertices[i], 1.f);
            vert /= vert.w;
            cx = (vert.x + 1) * 320;
            cy = (vert.y + 1) * 240;
            if (z_buffer[cx][cy] < vert.z) z_buffer[cx][cy] = vert.z;
        }

        for (int i=0; i<shape->vertices.size(); i++) {
            vert = MVPMatrix * glm::vec4(shape->vertices[i], 1.f);
            vert /= vert.w;
            cx = (vert.x + 1) * 320;
            cy = (vert.y + 1) * 240;
            if (std::fabs(z_buffer[cx][cy] - vert.z) < 0.00001f) {
                cv::Vec3b pixel = mapper.color_image.at<cv::Vec3b>(cy, cx);
                shape->colors[i] = glm::vec4(
//                        1.f, 0.f, 0.f,
                        pixel.val[2] / 255.f,
                        pixel.val[1] / 255.f,
                        pixel.val[0] / 255.f,
                        1.f
                );
            }
        }
        break;
    }
}
