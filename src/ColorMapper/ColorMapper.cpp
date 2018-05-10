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

    const std::string path_folder = "output/convert/";
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
    GLUnit u;
    int iterations;
    bool last_pass;
    printf("Iteration count: ");
    scanf("%d", &iterations);

    prepare_OGL(u);
    register_views(u);
    register_vertices(u);
    printf("\n[LOG] vertices registered.");

    for (int i=0; i<iterations; i++) {
        last_pass = (i + 1 == iterations);
        color_vertices(u, last_pass);
        if (!last_pass) {
            optimize_pose(u);
            printf("\n[LOG] Iteration %d finished.\n", i + 1);
        }
    }
    destroy_OGL(u);
}

void ColorMapper::prepare_OGL(GLUnit &u) {
    compileShader(&u.shader, "shader/depth.vert", "shader/depth.frag");

    glGenVertexArrays(1, &u.vao);
    glBindVertexArray(u.vao);
    glGenBuffers(2, u.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, u.vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * shape->vertices.size(), &shape->vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, u.vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * shape->faces.size(), &shape->faces[0], GL_STATIC_DRAW);
    glBindVertexArray(0);

    glGenFramebuffers(1, &u.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, u.fbo);
    glGenRenderbuffers(1, &u.rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, u.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, u.frameWidth, u.frameHeight);
//    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT32F, frameWidth, frameHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, u.rbo);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "INCOMPLETE FBO\n");
        exit(-1);
    }

    glDrawBuffer(GL_NONE);

    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClearDepth(1.f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
//    glEnable(GL_CULL_FACE);
//    glCullFace(GL_BACK);

    glViewport(0, 0, u.frameWidth, u.frameHeight);
}

void ColorMapper::destroy_OGL(GLUnit &u) {
    glDeleteBuffers(2, u.vbo);
    glDeleteVertexArrays(1, &u.vao);
    glDeleteRenderbuffers(1, &u.rbo);
    glDeleteFramebuffers(1, &u.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_CULL_FACE);
}

void ColorMapper::register_views(GLUnit &u) {
    cv::Mat screenshot_data;
    auto screenshot_raw = new GLfloat[u.frameWidth * u.frameHeight];

    best_views.clear();
    best_views.resize(shape->vertices.size(), -1.f);

    for (auto &mapper : map_units) {
        mapper.vertices.clear();
        glm::mat4 transform = glm::inverse(mapper.transform);
        glm::vec4 g;
        int cx, cy;

        glClear(GL_DEPTH_BUFFER_BIT);

        u.shader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(u.shader.ProgramId(), "transform"), 1, GL_FALSE, &transform[0][0]);

        glBindVertexArray(u.vao);
        glDrawElements(GL_TRIANGLES, shape->faces.size(), GL_UNSIGNED_INT, 0);
        u.shader.Deactivate();

        glReadPixels(0, 0, u.frameWidth, u.frameHeight, GL_DEPTH_COMPONENT, GL_FLOAT, screenshot_raw);
        screenshot_data = cv::Mat(u.frameHeight, u.frameWidth, CV_32F, screenshot_raw);

        for (int i=0; i<shape->vertices.size(); i++) {
            g = transform * glm::vec4(shape->vertices[i], 1.f);
            GLfloat z = .75f + g.z / 8.f;
            cx = u.SSAA * (g.x * u.f.x / g.z + u.c.x);
            cy = u.SSAA * (g.y * u.f.y / g.z + u.c.y);

            if (cx < 3 || cx + 3 > u.frameWidth || cy < 3 || cy + 3 > u.frameHeight) {
                continue;
            }

            float pixel = screenshot_data.at<float>(cy, cx);
            if (!(z < pixel + 0.0001f)) continue;

            float eyeVis = std::fabs(glm::dot(glm::normalize(glm::mat3(transform) * shape->normals[i]), glm::vec3(0.f, 0.f, 1.f)));
            if (best_views[i] < eyeVis) best_views[i] = eyeVis;
        }
    }

    delete []screenshot_raw;
}

void ColorMapper::register_vertices(GLUnit &u) {
    cv::Mat screenshot_data;
    auto screenshot_raw = new GLfloat[u.frameWidth * u.frameHeight];

    for (auto &mapper : map_units) {
        cv::Mat grad;
        glm::mat4 transform = glm::inverse(mapper.transform);
        glm::mat3 transform3(transform);
        glm::vec4 g;
        int cx, cy;

        mapper.vertices.clear();

        glClear(GL_DEPTH_BUFFER_BIT);
        u.shader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(u.shader.ProgramId(), "transform"), 1, GL_FALSE, &transform[0][0]);
        glBindVertexArray(u.vao);
        glDrawElements(GL_TRIANGLES, shape->faces.size(), GL_UNSIGNED_INT, 0);
        u.shader.Deactivate();

        glReadPixels(0, 0, u.frameWidth, u.frameHeight, GL_DEPTH_COMPONENT, GL_FLOAT, screenshot_raw);
        screenshot_data = cv::Mat(u.frameHeight, u.frameWidth, CV_32F, screenshot_raw);

        // Calculate gradient
        {
            cv::Mat gray, grad_x, grad_y;
            cv::normalize(screenshot_data, gray, 0, 1, cv::NORM_MINMAX);
            gray.convertTo(gray, CV_32F, 1.f, 0);

            cv::Scharr(gray, grad_x, -1, 0, 1);
            grad_x = cv::abs(grad_x);

            cv::Scharr(gray, grad_y, -1, 1, 0);
            grad_y = cv::abs(grad_y);

            cv::addWeighted(grad_x, 0.5, grad_y, 0.5, 0, grad);
        }

        for (int i=0; i<shape->vertices.size(); i++) {
            g = transform * glm::vec4(shape->vertices[i], 1.f);
            GLfloat z = .75f + g.z / 8.f;
            cx = u.SSAA * (g.x * u.f.x / g.z + u.c.x);
            cy = u.SSAA * (g.y * u.f.y / g.z + u.c.y);

            if (cx < 3 || cx + 3 > u.frameWidth || cy < 3 || cy + 3 > u.frameHeight) {
                continue;
            }

            float pixel = screenshot_data.at<float>(cy, cx);
            float gradient = grad.at<float>(cy, cx);
            if (gradient > 0.1) continue;

            float eyeVis = std::fabs(glm::dot(glm::normalize(transform3 * shape->normals[i]), glm::vec3(0.f, 0.f, 1.f)));
            if (eyeVis < best_views[i] - 0.1f) continue;
            if (z < pixel + 0.0001f) {
                mapper.vertices.push_back(i);
            }
        }
    }
    delete[]screenshot_raw;
}

void ColorMapper::color_vertices(GLUnit &u, bool need_color) {
    auto mapped_count = new int[shape->vertices.size()];
    memset(mapped_count, 0, sizeof(int) * shape->vertices.size());

    std::fill(shape->colors.begin(), shape->colors.end(), glm::vec4(0.f));

    grey_colors.clear();
    grey_colors.resize(shape->vertices.size());

    for (auto &mapper : map_units) {
        glm::mat4 transform = glm::inverse(mapper.transform);
        glm::vec4 g;
        int cx, cy;
        GLuint id;

        for (int i=0; i<mapper.vertices.size(); i++) {
            id = mapper.vertices[i];
            g = transform * glm::vec4(shape->vertices[id], 1.f);
            GLfloat z = .75f + g.z / 8.f;

            cx = g.x * u.f.x / g.z + u.c.x;
            cy = g.y * u.f.y / g.z + u.c.y;

            float pixel = mapper.grey_image.at<float>(cy, cx);
            grey_colors[id] *= mapped_count[id];
            grey_colors[id] += pixel;
            grey_colors[id] /= (mapped_count[id] + 1);

            if (need_color) {
                cv::Vec3f pixel_c = mapper.color_image.at<cv::Vec3f>(cy, cx);
                shape->colors[id] *= mapped_count[id];
                shape->colors[id] += glm::vec4(
                        pixel_c.val[2],
                        pixel_c.val[1],
                        pixel_c.val[0],
                        1.f
                );
                shape->colors[id] /= (mapped_count[id] + 1);
            }

            mapped_count[id]++;
        }
    }
    delete[]mapped_count;
}

void ColorMapper::optimize_pose(GLUnit &u) {
    for (auto &mapper : map_units) {
        glm::mat4 transform = glm::inverse(mapper.transform);
        glm::vec4 vert;
        int cx, cy;

        cv::Mat Jr, r, deltaX;
        cv::Mat _Jr;
        cv::Mat J_Gamma, Ju, Jg;
        glm::vec4 g;

        for (int i=0; i<mapper.vertices.size(); i++) {
            GLuint id = mapper.vertices[i];
            g = transform * glm::vec4(shape->vertices[id], 1.f);
            GLfloat z = .75f + vert.z / 8.f;

            Jg = (cv::Mat_<float>(4, 6) <<
                    0, g.z, -g.y, g.w, 0, 0,
                    -g.z, 0, g.x, 0, g.w, 0,
                    g.y, -g.x, 0, 0, 0, g.w,
                    0, 0, 0, 0, 0, 0
            );

            cx = g.x * u.f.x / g.z + u.c.x;
            cy = g.y * u.f.y / g.z + u.c.y;

            if (cx < 3 || cx + 3 > u.frameWidth / u.SSAA || cy < 3 || cy + 3 > u.frameHeight / u.SSAA) {
                continue;
            }

            Ju = (cv::Mat_<float>(2, 4) <<
                    u.f.x / g.z, 0, -g.x * u.f.x / g.z / g.z, 0,
                    0, u.f.y / g.z, -g.y * u.f.y / g.z / g.z, 0
            );
            float pixel = grey_colors[id] - mapper.grey_image.at<float>(cy, cx);

            J_Gamma = (cv::Mat_<float>(1, 2) <<
                    mapper.grad_x.at<float>(cy, cx),
                    mapper.grad_y.at<float>(cy, cx)
            );

            _Jr = -J_Gamma * Ju * Jg;

            std::cout
//                    << Ju << std::endl
//                    << Jg << std::endl
//                    << J_Gamma << std::endl
//                    << pixel << std::endl
//                    << _Jr << std::endl
                    ;
//            getchar();
            Jr.push_back(_Jr);
            r.push_back(pixel);
        }

        cv::Mat JrT = Jr.t();
        cv::Mat src1, src2;
        src1 = JrT * Jr;
        src2 = JrT * -r;
        cv::solve(src1, src2, deltaX);

        std::cout
//                << src1 << std::endl
//                << src2 << std::endl
                << deltaX << std::endl
                ;
//        getchar();

        float alpha_i = deltaX.at<float>(0, 0);
        float beta_i = deltaX.at<float>(1, 0);
        float gamma_i = deltaX.at<float>(2, 0);
        float ai = deltaX.at<float>(3, 0);
        float bi = deltaX.at<float>(4, 0);
        float ci = deltaX.at<float>(5, 0);

        glm::mat4 kx(
                glm::vec4(1.f, gamma_i, -beta_i, 0.f),
                glm::vec4(-gamma_i, 1.f, alpha_i, 0.f),
                glm::vec4(beta_i, -alpha_i, 1.f, 0.f),
                glm::vec4(ai, bi, ci, 1.f)
        );

        mapper.transform = kx * mapper.transform;
    }
}
