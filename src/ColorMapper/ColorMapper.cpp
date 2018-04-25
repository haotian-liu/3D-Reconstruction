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

bool ColorMapper::compileShader(ShaderProgram *shader, const std::string &vs, const std::string &fs) {
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

void ColorMapper::base_map() {
    const int frameWidth = 640, frameHeight = 480;

    const glm::mat4 projMatrix = glm::perspective(glm::radians(60.f), 640.f / 480, 0.001f, 10.f);

    GLuint fboMsaa, rboMsaa, vao, vbo[2];
    ShaderProgram shader;
    compileShader(&shader, "shader/depth.vert", "shader/depth.frag");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * shape->vertices.size(), &shape->vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * shape->faces.size(), &shape->faces[0], GL_STATIC_DRAW);
    glBindVertexArray(0);

    glGenFramebuffers(1, &fboMsaa);
    glBindFramebuffer(GL_FRAMEBUFFER, fboMsaa);
    glGenRenderbuffers(1, &rboMsaa);
    glBindRenderbuffer(GL_RENDERBUFFER, rboMsaa);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, frameWidth, frameHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboMsaa);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "INCOMPLETE MSAA FBO\n");
        exit(-1);
    }

    glDrawBuffer(GL_NONE);

    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClearDepth(1.f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glViewport(0, 0, frameWidth, frameHeight);

    cv::Mat screenshot_data;
    auto screenshot_raw = new GLfloat[frameWidth * frameHeight];

    auto mapped_count = new int[shape->vertices.size()];
    memset(mapped_count, 0, sizeof(int) * shape->vertices.size());

    for (const auto &mapper : map_units) {
        glm::mat4 transform = projMatrix * mapper.transform;
        glm::vec4 vert;
        int cx, cy;

        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        shader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(shader.ProgramId(), "transform"), 1, GL_FALSE, &transform[0][0]);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, shape->faces.size(), GL_UNSIGNED_INT, 0);
        shader.Deactivate();

        glReadPixels(0, 0, frameWidth, frameHeight, GL_DEPTH_COMPONENT, GL_FLOAT, screenshot_raw);
        screenshot_data = cv::Mat(frameHeight, frameWidth, CV_32F, screenshot_raw);

        for (int i=0; i<shape->vertices.size(); i++) {
            vert = transform * glm::vec4(shape->vertices[i], 1.f);
            GLfloat z = .5f + vert.z / vert.w / 2.f;
            vert /= vert.w;
            cx = (vert.x + 1) * frameWidth / 2;
            cy = (vert.y + 1) * frameHeight / 2;
            float pixel = screenshot_data.at<float>(cy, cx);
            printf("%f %f\n", pixel, z);
            if (std::fabs(pixel - z) < 0.00002f) {
                cx = (vert.x + 1) * 320;
                cy = (vert.y + 1) * 240;
                cv::Vec3b pixel = mapper.color_image.at<cv::Vec3b>(cy, cx);
//                shape->colors[i] *= mapped_count[i];
                shape->colors[i] = glm::vec4(
//                        1.f, 0.f, 0.f,
                        pixel.val[2] / 255.f,
                        pixel.val[1] / 255.f,
                        pixel.val[0] / 255.f,
                        1.f
                );
//                shape->colors[i] /= ++mapped_count[i];
            }
        }
        break;
    }

    delete[]mapped_count;
    delete[]screenshot_raw;
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteRenderbuffers(1, &rboMsaa);
    glDeleteFramebuffers(1, &fboMsaa);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
