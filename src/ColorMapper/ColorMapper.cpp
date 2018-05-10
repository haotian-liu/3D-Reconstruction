//
// Created by Haotian on 2018/3/22.
//

#include "ColorMapper.h"
#include <fstream>

void ColorMapper::map_color() {
    load_keyframes();
    load_images();
    int iterations = 5;
    for (int i=0; i<iterations; i++) {
        base_map(i + 1 == iterations);
        fprintf(stdout, "\n[LOG] Iteration %d finished.\n", i + 1);
    }
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

void ColorMapper::base_map(bool map_color) {
    const int SSAA = 2;
    const int frameWidth = 640 * SSAA, frameHeight = 480 * SSAA;
    const int imageHalfWidth = 320, imageHalfHeight = 240;

    const glm::mat4 projMatrix = glm::perspective(glm::radians(60.f), 640.f / 480, 0.001f, 10.f);

    GLuint fbo, rbo, vao, vbo[2];
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

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, frameWidth, frameHeight);
//    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT32F, frameWidth, frameHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

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

    glViewport(0, 0, frameWidth, frameHeight);

    cv::Mat screenshot_data;
    auto screenshot_raw = new GLfloat[frameWidth * frameHeight];


    if (!best_views.empty()) goto skip_best_view;


    best_views.clear();
    best_views.resize(shape->vertices.size(), -1.f);

    for (auto &mapper : map_units) {
        mapper.vertices.clear();
        glm::mat4 transform = glm::inverse(mapper.transform);
        glm::vec4 vert;
        int cx, cy;

        glClear(GL_DEPTH_BUFFER_BIT);

        shader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(shader.ProgramId(), "transform"), 1, GL_FALSE, &transform[0][0]);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, shape->faces.size(), GL_UNSIGNED_INT, 0);
        shader.Deactivate();

        glReadPixels(0, 0, frameWidth, frameHeight, GL_DEPTH_COMPONENT, GL_FLOAT, screenshot_raw);
        screenshot_data = cv::Mat(frameHeight, frameWidth, CV_32F, screenshot_raw);

//        cv::normalize(screenshot_data, screenshot_data, 0, 1, cv::NORM_MINMAX);
//        cv::imshow("window", screenshot_data);
//        cv::waitKey(0);
        const glm::vec2 f(525.f, 525.f), c(319.5f, 239.5f);
        glm::vec4 g;

        for (int i=0; i<shape->vertices.size(); i++) {
            vert = transform * glm::vec4(shape->vertices[i], 1.f);
            g = vert;
//            vert /= vert.w;
            GLfloat z = .75f + vert.z / 8.f;
//            cx = (vert.x + 1) * frameWidth / 2;
//            cy = (vert.y + 1) * frameHeight / 2;
            cx = SSAA * (g.x * f.x / g.z + c.x);
            cy = SSAA * (g.y * f.y / g.z + c.y);

            if (cx < 3 || cx + 3 > frameWidth || cy < 3 || cy + 3 > frameHeight) {
                continue;
            }

            float pixel = screenshot_data.at<float>(cy, cx);
            if (!(z < pixel + 0.0001f)) continue;

            float eyeVis = std::fabs(glm::dot(glm::mat3(transform) * shape->normals[i], glm::vec3(0.f, 0.f, 1.f)));
            if (best_views[i] < eyeVis) best_views[i] = eyeVis;
        }
    }

    skip_best_view:
    // test visibility




    auto mapped_count = new int[shape->vertices.size()];
    memset(mapped_count, 0, sizeof(int) * shape->vertices.size());

    std::fill(shape->colors.begin(), shape->colors.end(), glm::vec4(0.f));

    grey_colors.clear();
    grey_colors.resize(shape->vertices.size());

    for (auto &mapper : map_units) {
        mapper.vertices.clear();
        glm::mat4 transform = glm::inverse(mapper.transform);
        glm::vec4 vert;
        int cx, cy;

        glClear(GL_DEPTH_BUFFER_BIT);

        shader.Activate();
        glUniformMatrix4fv(glGetUniformLocation(shader.ProgramId(), "transform"), 1, GL_FALSE, &transform[0][0]);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, shape->faces.size(), GL_UNSIGNED_INT, 0);
        shader.Deactivate();

        glReadPixels(0, 0, frameWidth, frameHeight, GL_DEPTH_COMPONENT, GL_FLOAT, screenshot_raw);
        screenshot_data = cv::Mat(frameHeight, frameWidth, CV_32F, screenshot_raw);

//        cv::normalize(screenshot_data, screenshot_data, 0, 1, cv::NORM_MINMAX);
//        cv::imshow("window", screenshot_data);
//        cv::waitKey(0);

        cv::Mat gray, grad, grad_x, grad_y;
        cv::normalize(screenshot_data, gray, 0, 1, cv::NORM_MINMAX);
        gray.convertTo(gray, CV_32F, 1.f, 0);

        cv::Scharr(gray, grad_x, -1, 0, 1);
        grad_x = cv::abs(grad_x);

        cv::Scharr(gray, grad_y, -1, 1, 0);
        grad_y = cv::abs(grad_y);

        cv::addWeighted(grad_x, 0.5, grad_y, 0.5, 0, grad);

        const glm::vec2 f(525.f, 525.f), c(319.5f, 239.5f);
        glm::vec4 g;

        for (int i=0; i<shape->vertices.size(); i++) {
//            glm::vec4 tmp_vert = mapper.transform * glm::vec4(shape->vertices[i], 1.f);
//            vert.x = -projMatrix[0][0] * tmp_vert.x / tmp_vert.z;
//            vert.y = -projMatrix[1][1] * tmp_vert.y / tmp_vert.z;
//            vert.z = -(projMatrix[2][2] * tmp_vert.z + projMatrix[3][2]) / tmp_vert.z;
//            vert.w = projMatrix[2][3] * tmp_vert.z;
            vert = transform * glm::vec4(shape->vertices[i], 1.f);
            g = vert;
//            vert /= vert.w;
            GLfloat z = .75f + vert.z / 8.f;
//            cx = (vert.x + 1) * frameWidth / 2;
//            cy = (vert.y + 1) * frameHeight / 2;
            cx = SSAA * (g.x * f.x / g.z + c.x);
            cy = SSAA * (g.y * f.y / g.z + c.y);

            if (cx < 3 || cx + 3 > frameWidth || cy < 3 || cy + 3 > frameHeight) {
                continue;
            }

            float pixel = screenshot_data.at<float>(cy, cx);
            float gradient = grad.at<float>(cy, cx);
            if (gradient > 0.1) continue;
//
            float eyeVis = std::fabs(glm::dot(glm::mat3(transform) * shape->normals[i], glm::vec3(0.f, 0.f, 1.f)));
            if (eyeVis < best_views[i] - 0.1f) continue;
//            if (std::fabs(pixel - z) < 0.00002f) {
            if (z < pixel + 0.0001f) {
                cx = g.x * f.x / g.z + c.x;
                cy = g.y * f.y / g.z + c.y;

//                float pixel = mapper.grey_image.at<uchar>(cy, cx) / 255.f;
                float pixel = mapper.grey_image.at<float>(cy, cx);
//                if (i == 5) {
//                    printf("%f %f %d", pixel, grey_colors[i], mapped_count[i]);
//                    getchar();
//                }
                grey_colors[i] *= mapped_count[i];
                grey_colors[i] += pixel;
                grey_colors[i] /= (mapped_count[i] + 1);

//                if (i == 5) {
//                    printf("%f %f %d", pixel, grey_colors[i], mapped_count[i]);
//                    getchar();
//                }

                if (map_color) {
                    cv::Vec3f pixel_c = mapper.color_image.at<cv::Vec3f>(cy, cx);
                    shape->colors[i] *= mapped_count[i];
                    shape->colors[i] += glm::vec4(
//                        1.f, 0.f, 0.f,
                            pixel_c.val[2],
                            pixel_c.val[1],
                            pixel_c.val[0],
                            1.f
                    );
                    shape->colors[i] /= (mapped_count[i] + 1);
                }

                mapped_count[i]++;
                mapper.vertices.push_back(i);
            }
        }
    }

    ///////////////////////////////////////

    delete[]mapped_count;
    delete[]screenshot_raw;
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_CULL_FACE);

    if (map_color) { return; }

    ///////////////////////////////////////
    ////////// Optimize camera Pose ///////

    for (auto &mapper : map_units) {
        glm::mat4 transform = glm::inverse(mapper.transform);
        glm::vec4 vert;
        int cx, cy;

//        cv::imshow("grad_x", grad_x);
//        cv::waitKey(0);
//        cv::imshow("grad_y", grad_y);
//        cv::waitKey(0);

        cv::Mat Jr, r, deltaX;
        cv::Mat _Jr;
        cv::Mat J_Gamma, Ju, Jg;
        const glm::vec2 f(525.f, 525.f), c(319.5f, 239.5f);
        glm::vec4 g;

        for (int i=0; i<mapper.vertices.size(); i++) {
            GLuint id = mapper.vertices[i];
            g = transform * glm::vec4(shape->vertices[id], 1.f);
//            vert /= vert.w;
            GLfloat z = .75f + vert.z / 8.f;
//            cx = (vert.x + 1) * frameWidth / 2;
//            cy = (vert.y + 1) * frameHeight / 2;

            Jg = (cv::Mat_<float>(4, 6) <<
                    0, g.z, -g.y, g.w, 0, 0,
                    -g.z, 0, g.x, 0, g.w, 0,
                    g.y, -g.x, 0, 0, 0, g.w,
                    0, 0, 0, 0, 0, 0
            );

            cx = g.x * f.x / g.z + c.x;
            cy = g.y * f.y / g.z + c.y;

            if (cx < 3 || cx + 3 > frameWidth / SSAA || cy < 3 || cy + 3 > frameHeight / SSAA) {
                continue;
            }

//            Ju = (cv::Mat_<float>(2, 4) <<
//                    -imageHalfWidth * projMatrix[0][0] / tmp_vert.z, 0, imageHalfWidth * projMatrix[0][0] * (1.0 + tmp_vert.x) / tmp_vert.z / tmp_vert.z, 0,
//                    0, -imageHalfHeight * projMatrix[1][1] / tmp_vert.z, imageHalfHeight * projMatrix[1][1] * (1.0 + tmp_vert.y) / tmp_vert.z / tmp_vert.z, 0
//            );
//
            Ju = (cv::Mat_<float>(2, 4) <<
                    f.x / g.z, 0, -g.x * f.x / g.z / g.z, 0,
                    0, f.y / g.z, -g.y * f.y / g.z / g.z, 0
            );
//            float pixel = grey_colors[id] - mapper.grey_image.at<uchar>(cy, cx) / 255.f;
            float pixel = grey_colors[id] - mapper.grey_image.at<float>(cy, cx);
//            printf("%f - %f = %f", mapper.grey_image.at<uchar>(cy, cx) / 255.f, grey_colors[id], pixel);getchar();

            J_Gamma = (cv::Mat_<float>(1, 2) <<
//                    mapper.grad_x.at<uchar>(cy, cx) / 255.0,
//                    mapper.grad_y.at<uchar>(cy, cx) / 255.0
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

//        std::cout << alpha_i << " "
//                  << beta_i << " "
//                  << gamma_i << " "
//                  << ai << " "
//                  << bi << " "
//                  << ci << " "
//                  << std::endl;

        glm::mat4 kx(
                glm::vec4(1.f, gamma_i, -beta_i, 0.f),
                glm::vec4(-gamma_i, 1.f, alpha_i, 0.f),
                glm::vec4(beta_i, -alpha_i, 1.f, 0.f),
                glm::vec4(ai, bi, ci, 1.f)
        );

        mapper.transform = kx * mapper.transform;
    }
}
