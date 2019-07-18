//
// Created by Haotian on 2018/3/22.
//

#include "ColorMapper.h"
#include <fstream>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Sparse>
#include <Timer/Timer.h>

using Eigen::MatrixXf;
using Eigen::VectorXf;
using Eigen::VectorXi;
using Eigen::SparseMatrix;
using Eigen::SparseVector;

typedef Eigen::Triplet<float> Triplet;

void ColorMapper::map_color() {
    load_keyframes();
    load_images();
//    base_map();
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
    int iterations, optimize_flag;
    bool last_pass;
    Timer timer;
    printf("Iteration count: ");
    scanf("%d", &iterations);
    printf("Optimize FLAG (1=FULL,2=POSE,3=PAPER_POSE): ");
    scanf("%d", &optimize_flag);

    prepare_OGL(u);
    register_depths(u);
//    register_views(u);
    if (optimize_flag == 2) { register_views(u); register_vertices_pose_only(u); }
    else register_vertices(u);
    destroy_OGL(u);
    printf("[LOG] vertices registered.\n");

    for (iteration=0; iteration<iterations; iteration++) {
        last_pass = (iteration + 1 == iterations);

        if (optimize_flag == 1) color_vertices(u, last_pass);
        else if (optimize_flag == 2) color_vertices_pose_only(u, last_pass);
        else color_vertices_pose_paper(u, last_pass);

        if (!last_pass) {
            timer.start();
            if (optimize_flag == 1) optimize_pose(u);
            else if (optimize_flag == 2) optimize_pose_only(u);
            else optimize_pose_paper(u);
            timer.stop();
            printf("[LOG] Iteration %d finished in %lld ms.\n", iteration + 1, timer.elasped());
        }
    }
    printf("[LOG] Color mapped.\n");
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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glViewport(0, 0, u.frameWidth, u.frameHeight);
}

void ColorMapper::destroy_OGL(GLUnit &u) {
    glDeleteBuffers(2, u.vbo);
    glDeleteVertexArrays(1, &u.vao);
    glDeleteRenderbuffers(1, &u.rbo);
    glDeleteFramebuffers(1, &u.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glFrontFace(GL_CCW);
    glDisable(GL_CULL_FACE);
}

void ColorMapper::register_views(GLUnit &u) {
    cv::Mat screenshot_data;
    auto screenshot_raw = new GLfloat[u.frameWidth * u.frameHeight];

    best_views.clear();
    best_views.resize(shape->vertices.size(), -1.f);
    best_view_dirs.resize(shape->vertices.size());

    for (auto &mapper : map_units) {
        mapper.vertices.clear();
        glm::mat4 transform = mapper.transform;
        glm::mat3 transform3x3(transform);
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

            if (cx < 6 || cx + 6 > u.frameWidth || cy < 6 || cy + 6 > u.frameHeight) {
                continue;
            }

//            float pixel = screenshot_data.at<float>(cy, cx);
            float pixel = getColorSubpix(screenshot_data, cv::Point2f(cx, cy));
            if (!within_depth(i, pixel, z)) continue;

            float eyeVis = glm::dot(transform3x3 * shape->normals[i], glm::vec3(0.f, 0.f, -1.f));
            if (best_views[i] < eyeVis) {
                best_views[i] = eyeVis;
                best_view_dirs[i] = transform3x3 * shape->normals[i];
            }
        }
    }

    delete []screenshot_raw;
}

void ColorMapper::register_depths(GLUnit &u) {
    cv::Mat screenshot_data;
    auto screenshot_raw = new GLfloat[u.frameWidth * u.frameHeight];

    best_depths.clear();
    best_depths.resize(shape->vertices.size(), 0.01);

    for (auto &mapper : map_units) {
        mapper.vertices.clear();
        glm::mat4 transform = mapper.transform;
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

            if (cx < 6 || cx + 6 > u.frameWidth || cy < 6 || cy + 6 > u.frameHeight) {
                continue;
            }

//            float pixel = screenshot_data.at<float>(cy, cx);
            float pixel = getColorSubpix(screenshot_data, cv::Point2f(cx, cy));
            if (std::fabs(z - pixel) < best_depths[i]) best_depths[i] = std::fabs(z - pixel);
        }
    }

    delete []screenshot_raw;
}

void ColorMapper::register_vertices(GLUnit &u) {
    cv::Mat screenshot_data;
    auto screenshot_raw = new GLfloat[u.frameWidth * u.frameHeight];

    for (auto &mapper : map_units) {
        cv::Mat grad;
        glm::mat4 transform = mapper.transform;
        glm::mat3 transform3(transform);
        glm::vec4 g;
        float cx, cy;

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

            if (cx < 6 || cx + 6 > u.frameWidth || cy < 6 || cy + 6 > u.frameHeight) {
                continue;
            }

//            float pixel = screenshot_data.at<float>(cy, cx);
            float pixel = getColorSubpix(screenshot_data, cv::Point2f(cx, cy));
            float gradient = grad.at<float>(cy, cx);
            if (gradient > 0.1) continue;

//            float eyeVis = glm::dot(transform3 * shape->normals[i], best_view_dirs[i]);
//            if (eyeVis < 0.9f) continue;
            if (within_depth(i, pixel, z)) {
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
        glm::mat4 transform = mapper.transform;
        glm::vec4 g;
        float cx, cy;
        GLuint id;

        for (int i=0; i<mapper.vertices.size(); i++) {
            id = mapper.vertices[i];
            g = transform * glm::vec4(shape->vertices[id], 1.f);
            GLfloat z = .75f + g.z / 8.f;

            cx = g.x * u.f.x / g.z + u.c.x;
            cy = g.y * u.f.y / g.z + u.c.y;

            if (std::isnan(cx) || std::isnan(cy)) {
                continue;
            }
            if (cx < 3 || cx + 3 > u.GLWidth || cy < 3 || cy + 3 > u.GLHeight) {
                continue;
            }

            int icx = cx / 64, icy = cy / 64;

//            if (iteration) printf("%f %f ", cx, cy);

            glm::vec2 lerp = bilerp(
                    mapper.control_vertices[icx][icy],
                    mapper.control_vertices[icx+1][icy],
                    mapper.control_vertices[icx][icy+1],
                    mapper.control_vertices[icx+1][icy+1],
                    cx, cy
            );

            cx += lerp.x;
            cy += lerp.y;

            if (cx < 3 || cx + 3 > u.GLWidth || cy < 3 || cy + 3 > u.GLHeight) {
                continue;
            }

//            if (iteration) printf("%f %f ", cx, cy);
//            if (iteration) getchar();

//            float pixel = mapper.grey_image.at<float>(cy, cx);
            float pixel = getColorSubpix(mapper.grey_image, cv::Point2f(cx, cy));
            grey_colors[id] += pixel;

            if (need_color) {
                cv::Vec3f pixel_c = mapper.color_image.at<cv::Vec3f>(cy, cx);
                shape->colors[id] += glm::vec4(
                        pixel_c.val[2],
                        pixel_c.val[1],
                        pixel_c.val[0],
                        1.f
                );
            }

            mapped_count[id]++;
        }
    }
    for (int i=0; i<shape->colors.size(); i++) {
        grey_colors[i] /= mapped_count[i];
    }
    if (need_color) {
        for (int i=0; i<shape->colors.size(); i++) {
            shape->colors[i] /= mapped_count[i];
        }
    }
    delete[]mapped_count;
}

void ColorMapper::color_vertices_pose_paper(GLUnit &u, bool need_color) {
    auto mapped_count = new int[shape->vertices.size()];
    memset(mapped_count, 0, sizeof(int) * shape->vertices.size());

    std::fill(shape->colors.begin(), shape->colors.end(), glm::vec4(0.f));

    grey_colors.clear();
    grey_colors.resize(shape->vertices.size());

    for (auto &mapper : map_units) {
        glm::mat4 transform = mapper.transform;
        glm::vec4 g;
        float cx, cy;
        GLuint id;

        for (int i=0; i<mapper.vertices.size(); i++) {
            id = mapper.vertices[i];
            g = transform * glm::vec4(shape->vertices[id], 1.f);
            GLfloat z = .75f + g.z / 8.f;

            cx = g.x * u.f.x / g.z + u.c.x;
            cy = g.y * u.f.y / g.z + u.c.y;

            if (std::isnan(cx) || std::isnan(cy)) {
                continue;
            }

            if (cx < 3 || cx + 3 > u.GLWidth || cy < 3 || cy + 3 > u.GLHeight) {
                continue;
            }

//            float pixel = mapper.grey_image.at<float>(cy, cx);
            float pixel = getColorSubpix(mapper.grey_image, cv::Point2f(cx, cy));
            grey_colors[id] += pixel;

            if (need_color) {
                cv::Vec3f pixel_c = mapper.color_image.at<cv::Vec3f>(cy, cx);
                shape->colors[id] += glm::vec4(
                        pixel_c.val[2],
                        pixel_c.val[1],
                        pixel_c.val[0],
                        1.f
                );
            }

            mapped_count[id]++;
        }
    }
    for (int i=0; i<shape->colors.size(); i++) {
        grey_colors[i] /= mapped_count[i];
    }
    if (need_color) {
        for (int i=0; i<shape->colors.size(); i++) {
            shape->colors[i] /= mapped_count[i];
        }
    }
    delete[]mapped_count;
}

void ColorMapper::optimize_pose(GLUnit &u) {
#pragma omp parallel for
    for (int i=0; i<map_units.size(); i++) {
        auto &mapper = map_units[i];
        glm::mat4 transform = mapper.transform;
        glm::vec4 vert;
        float cx, cy;

        MatrixXf Jg(4, 6), Ju(2, 4), J_F(2, 2), _J_F(2, 2), J_Gamma(1, 2),
                _Jr(1, 6), Jr(mapper.vertices.size(), 6);
        std::vector<Triplet> tripletList;
        tripletList.reserve(mapper.vertices.size());
        VectorXf r(mapper.vertices.size());
        glm::vec4 g;
        int vert_count = 0;

        for (int i=0; i<mapper.vertices.size(); i++) {
            GLuint id = mapper.vertices[i];
            g = transform * glm::vec4(shape->vertices[id], 1.f);
            GLfloat z = .75f + vert.z / 8.f;

            Jg <<
               0, g.z, -g.y, g.w, 0, 0,
                    -g.z, 0, g.x, 0, g.w, 0,
                    g.y, -g.x, 0, 0, 0, g.w,
                    0, 0, 0, 0, 0, 0
                    ;

            cx = g.x * u.f.x / g.z + u.c.x;
            cy = g.y * u.f.y / g.z + u.c.y;

            if (std::isnan(cx) || std::isnan(cy)) {
                continue;
            }

            if (cx < 3 || cx + 3 > u.GLWidth || cy < 3 || cy + 3 > u.GLHeight) {
                continue;
            }

            Ju <<
               u.f.x / g.z, 0, -g.x * u.f.x / g.z / g.z, 0,
                    0, u.f.y / g.z, -g.y * u.f.y / g.z / g.z, 0
                    ;

            J_F.setIdentity();
            int icx = cx / 64, icy = cy / 64;
            glm::vec2 lerp = bilerp(
                    mapper.control_vertices[icx][icy],
                    mapper.control_vertices[icx+1][icy],
                    mapper.control_vertices[icx][icy+1],
                    mapper.control_vertices[icx+1][icy+1],
                    cx, cy
            );
            glm::mat2 lerp_grad = bilerp_gradient(
                    mapper.control_vertices[icx][icy],
                    mapper.control_vertices[icx+1][icy],
                    mapper.control_vertices[icx][icy+1],
                    mapper.control_vertices[icx+1][icy+1],
                    cx, cy
            );
            //

            _J_F << lerp_grad[0][0], lerp_grad[0][1], lerp_grad[1][0], lerp_grad[1][1];
            J_F += _J_F;


            float ccx = std::fmod(cx, 64) / 64.0, ccy = std::fmod(cy, 64) / 64.0;
            int f1_id = icx * 17 + icy;
            int f2_id = (1+icx) * 17 + icy;
            int f3_id = icx * 17 + (1+icy);
            int f4_id = (1+icx) * 17 + (1+icy);

            cx += lerp.x;
            cy += lerp.y;
            if (cx < 3 || cx + 3 > u.GLWidth || cy < 3 || cy + 3 > u.GLHeight) {
                continue;
            }

            float pixel = grey_colors[id] - getColorSubpix(mapper.grey_image, cv::Point2f(cx, cy));

            J_Gamma <<
//                    mapper.grad_x.at<float>(cy, cx),
//                    mapper.grad_y.at<float>(cy, cx),
                    getColorSubpix(mapper.grad_x, cv::Point2f(cx, cy)),
                    getColorSubpix(mapper.grad_y, cv::Point2f(cx, cy))
                    ;

            _Jr = J_Gamma * J_F * Ju * Jg;

            float coeff = 1.f;

            tripletList.push_back(Triplet(vert_count, f1_id*2,
                                          coeff * -(1-ccx)*(1-ccy) * J_Gamma(0)));
            tripletList.push_back(Triplet(vert_count, f1_id*2+1,
                                          coeff * -(1-ccx)*(1-ccy) * J_Gamma(1)));
            tripletList.push_back(Triplet(vert_count, f2_id*2,
                                          coeff * -ccx*(1-ccy) * J_Gamma(0)));
            tripletList.push_back(Triplet(vert_count, f2_id*2+1,
                                          coeff * -ccx*(1-ccy) * J_Gamma(1)));
            tripletList.push_back(Triplet(vert_count, f3_id*2,
                                          coeff * -(1-ccx)*ccy * J_Gamma(0)));
            tripletList.push_back(Triplet(vert_count, f3_id*2+1,
                                          coeff * -(1-ccx)*ccy * J_Gamma(1)));
            tripletList.push_back(Triplet(vert_count, f4_id*2,
                                          coeff * -ccx*ccy * J_Gamma(0)));
            tripletList.push_back(Triplet(vert_count, f4_id*2+1,
                                          coeff * -ccx*ccy * J_Gamma(1)));

            std::cout
//                    << Ju << std::endl
//                    << Jg << std::endl
//                    << J_Gamma << std::endl
//                    << pixel << std::endl
//                    << _Jr << std::endl
                    ;
//            getchar();
            Jr.row(vert_count) = _Jr;
            r(vert_count) = pixel;
            ++vert_count;
        }

        r = r.topRows(vert_count);

        MatrixXf src1_6(6, 6);
        VectorXf src2_6(6);
        {
            Jr = Jr.topRows(vert_count);
            MatrixXf JrT = Jr.transpose();
            src1_6 = JrT * Jr;
            src2_6 = JrT * -r;
        }
        MatrixXf src1_714(714, 714);
        VectorXf src2_714(714);
        {
            SparseMatrix<float, Eigen::RowMajor> S_Jr(vert_count, 714);
            SparseMatrix<float> JrT;
            S_Jr.setFromTriplets(tripletList.begin(), tripletList.end());
            JrT = S_Jr.transpose();
            src1_714 = JrT * S_Jr;
            src2_714 = JrT * -r;
        }
        MatrixXf src1 = MatrixXf::Zero(720, 720);
        VectorXf src2(720), deltaX(720);

        src1.block<6,6>(0, 0) = src1_6;
        src1.block<714,714>(6, 6) = src1_714;

        src2.segment<6>(0) = src2_6;
        src2.segment<714>(6) = src2_714;

        float lambda = 0.1;

        for (int cx=0; cx<21; cx++) {
            for (int cy=0; cy<17; cy++) {
                int i = cx * 17 + cy;
                src2(6 + i * 2) -= lambda * mapper.control_vertices[cx][cy].x;
                src2(6 + i * 2 + 1) -= lambda * mapper.control_vertices[cx][cy].y;
            }
        }

        deltaX = src1.ldlt().solve(src2);
//        deltaX = Jr.colPivHouseholderQr().solve(-r);

//        std::cout << vert_count << std::endl;

        bool is_nan = false;
        for (int i=0; i<720; i++) {
            if (std::isnan(deltaX(i))) { is_nan = true; }
        }
        if (is_nan) { continue; }

        for (int cx=0; cx<21; cx++) {
            for (int cy=0; cy<17; cy++) {
                int i = cx * 17 + cy;
                mapper.control_vertices[cx][cy] += glm::vec2(
                        deltaX(6 + i * 2),
                        deltaX(6 + i * 2 + 1)
                );
                if (glm::length(mapper.control_vertices[cx][cy]) > 5.f) mapper.control_vertices[cx][cy] = glm::vec2(0.f);
            }
        }

        std::cout
//                << src1 << std::endl
//                << src2 << std::endl
//                << deltaX << std::endl << std::endl
                ;
//        getchar();

        float alpha_i = deltaX(0);
        float beta_i = deltaX(1);
        float gamma_i = deltaX(2);
        float ai = deltaX(3);
        float bi = deltaX(4);
        float ci = deltaX(5);

        glm::mat4 kx(
                glm::vec4(1.f, gamma_i, -beta_i, 0.f),
                glm::vec4(-gamma_i, 1.f, alpha_i, 0.f),
                glm::vec4(beta_i, -alpha_i, 1.f, 0.f),
                glm::vec4(ai, bi, ci, 1.f)
        );

        mapper.transform = kx * mapper.transform;
    }
}

void ColorMapper::optimize_pose_paper(GLUnit &u) {
#pragma omp parallel for
    for (int i=0; i<map_units.size(); i++) {
        auto &mapper = map_units[i];
        glm::mat4 transform = mapper.transform;
        glm::vec4 vert;
        float cx, cy;

        MatrixXf Jg(4, 6), Ju(2, 4), J_Gamma(1, 2), _Jr(1, 6), Jr(mapper.vertices.size(), 6);
        VectorXf r(mapper.vertices.size());
        glm::vec4 g;
        int vert_count = 0;

        for (int i=0; i<mapper.vertices.size(); i++) {
            GLuint id = mapper.vertices[i];
            g = transform * glm::vec4(shape->vertices[id], 1.f);
            GLfloat z = .75f + vert.z / 8.f;

            Jg <<
               0, g.z, -g.y, g.w, 0, 0,
                    -g.z, 0, g.x, 0, g.w, 0,
                    g.y, -g.x, 0, 0, 0, g.w,
                    0, 0, 0, 0, 0, 0
                    ;

            cx = g.x * u.f.x / g.z + u.c.x;
            cy = g.y * u.f.y / g.z + u.c.y;

            if (std::isnan(cx) || std::isnan(cy)) {
                continue;
            }

            if (cx < 3 || cx + 3 > u.GLWidth || cy < 3 || cy + 3 > u.GLHeight) {
                continue;
            }

            Ju <<
               u.f.x / g.z, 0, -g.x * u.f.x / g.z / g.z, 0,
                    0, u.f.y / g.z, -g.y * u.f.y / g.z / g.z, 0
                    ;

//            float pixel = grey_colors[id] - mapper.grey_image.at<float>(cy, cx);
            float pixel = grey_colors[id] - getColorSubpix(mapper.grey_image, cv::Point2f(cx, cy));

            J_Gamma <<
                    mapper.grad_x.at<float>(cy, cx),
                    mapper.grad_y.at<float>(cy, cx)
                    ;

            _Jr = J_Gamma * Ju * Jg;

            std::cout
//                    << Ju << std::endl
//                    << Jg << std::endl
//                    << J_Gamma << std::endl
//                    << pixel << std::endl
//                    << _Jr << std::endl
                    ;
//            getchar();
            Jr.row(vert_count) = _Jr;
            r(vert_count) = pixel;
            ++vert_count;
        }

        if (!vert_count) continue;

        r = r.topRows(vert_count);
        Jr = Jr.topRows(vert_count);

        MatrixXf src1(6, 6);
        VectorXf src2(6), deltaX(7);
        MatrixXf JrT = Jr.transpose();
        src1 = JrT * Jr;
        src2 = JrT * -r;

        deltaX = src1.ldlt().solve(src2);
//        deltaX = Jr.colPivHouseholderQr().solve(-r);

//        std::cout << vert_count << std::endl;

        bool is_nan = false;
        for (int i=0; i<6; i++) {
            if (std::isnan(deltaX(i))) { is_nan = true; }
        }
        if (is_nan) { continue; }

        std::cout
//                << src1 << std::endl
//                << src2 << std::endl
//                << deltaX << std::endl << std::endl
                ;
//        getchar();

        float alpha_i = deltaX(0);
        float beta_i = deltaX(1);
        float gamma_i = deltaX(2);
        float ai = deltaX(3);
        float bi = deltaX(4);
        float ci = deltaX(5);

        glm::mat4 kx(
                glm::vec4(1.f, gamma_i, -beta_i, 0.f),
                glm::vec4(-gamma_i, 1.f, alpha_i, 0.f),
                glm::vec4(beta_i, -alpha_i, 1.f, 0.f),
                glm::vec4(ai, bi, ci, 1.f)
        );

        mapper.transform = kx * mapper.transform;
    }
}

glm::vec2 ColorMapper::bilerp(const glm::vec2 &v1, const glm::vec2 &v2, const glm::vec2 &v3, const glm::vec2 &v4,
                              float cx, float cy) const {
    float ccx = std::fmod(cx, 64) / 64.0, ccy = std::fmod(cy, 64) / 64.0;
    return (1-ccx)*(1-ccy) * v1 + ccx*(1-ccy) * v2 + (1-ccx)*ccy * v3 + ccx*ccy * v4;
}

glm::mat2
ColorMapper::bilerp_gradient(const glm::vec2 &v1, const glm::vec2 &v2, const glm::vec2 &v3, const glm::vec2 &v4,
                             float cx, float cy) const {
    float ccx = std::fmod(cx, 64) / 64.0, ccy = std::fmod(cy, 64) / 64.0;
    return glm::mat2(
            (ccy-1)*(v1-v2) + ccy*(v4-v3),
            (ccx-1)*(v1-v3) + ccx*(v4-v2)
    ) / 64.f;
}

float ColorMapper::getColorSubpix(const cv::Mat &img, cv::Point2f pt) {
    assert(!img.empty());
    assert(img.channels() == 1);

    auto x = (int)pt.x;
    auto y = (int)pt.y;

    int x0 = x;
    int x1 = x+1;
    int y0 = y;
    int y1 = y+1;

    float a = pt.x - (float)x;
    float c = pt.y - (float)y;

    return (img.at<float>(y0, x0) * (1.f - a) + img.at<float>(y0, x1) * a) * (1.f - c)
                             + (img.at<float>(y1, x0) * (1.f - a) + img.at<float>(y1, x1) * a) * c;
}

void ColorMapper::register_vertices_pose_only(GLUnit &u) {
    cv::Mat screenshot_data;
    auto screenshot_raw = new GLfloat[u.frameWidth * u.frameHeight];

    for (auto &mapper : map_units) {
        cv::Mat grad;
        glm::mat4 transform = mapper.transform;
        glm::mat3 transform3(transform);
        glm::vec4 g;
        float cx, cy;

        mapper.vertices_po.clear();
        mapper.vertices_po.resize((2 * mapper.v_rows - 1) * (2 * mapper.v_cols - 1));
        mapper.transform_po.clear();
        for (int i=0; i<(2 * mapper.v_rows - 1) * (2 * mapper.v_cols - 1); i++) {
            mapper.transform_po.push_back(mapper.transform);
        }
        int width = u.frameWidth / mapper.v_cols;
        int height = u.frameHeight / mapper.v_rows;

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

            if (cx < 6 || cx + 6 > u.frameWidth || cy < 6 || cy + 6 > u.frameHeight) {
                continue;
            }

//            float pixel = screenshot_data.at<float>(cy, cx);
            float pixel = getColorSubpix(screenshot_data, cv::Point2f(cx, cy));
            float gradient = grad.at<float>(cy, cx);
            if (gradient > 0.1) continue;

            float eyeVis = glm::dot(transform3 * shape->normals[i], best_view_dirs[i]);
            if (eyeVis < 0.9f) continue;
            if (within_depth(i, pixel, z)) {
                int col = cx / (width / 2);
                int row = cy / (height / 2);

                for (int dx = -1; dx <= 0; dx++) {
                    for (int dy = -1; dy <= 0; dy++) {
                        if (dx + col < 0) continue;
                        if (dy + row < 0) continue;
                        if (dx + col >= 2 * mapper.v_cols - 1) continue;
                        if (dy + row >= 2 * mapper.v_rows - 1) continue;
                        mapper.vertices_po[(dy + row) * (2 * mapper.v_cols - 1) + (dx + col)].push_back(i);
                    }
                }
            }
        }
    }
    delete[]screenshot_raw;
}

void ColorMapper::color_vertices_pose_only(GLUnit &u, bool need_color) {
    auto mapped_count = new int[shape->vertices.size()];
    memset(mapped_count, 0, sizeof(int) * shape->vertices.size());

    if (need_color) {
        std::fill(shape->colors.begin(), shape->colors.end(), glm::vec4(0.f));
    }

    if (grey_colors.capacity() != shape->vertices.size()) {
        grey_colors.resize(shape->vertices.size(), 0.f);
    }
    grey_colors.clear();

    for (auto &mapper : map_units) {
        glm::vec4 g;
        float cx, cy;
        GLuint id;

        for (int j=0; j<mapper.vertices_po.size(); j++) {
            auto &verts = mapper.vertices_po[j];
            glm::mat4 transform = mapper.transform_po[j];
            for (int i = 0; i < verts.size(); i++) {
                id = verts[i];
                g = transform * glm::vec4(shape->vertices[id], 1.f);
                GLfloat z = .75f + g.z / 8.f;

                cx = g.x * u.f.x / g.z + u.c.x;
                cy = g.y * u.f.y / g.z + u.c.y;

                if (std::isnan(cx) || std::isnan(cy)) {
                    continue;
                }

                if (cx < 3 || cx + 3 > u.GLWidth || cy < 3 || cy + 3 > u.GLHeight) {
                    continue;
                }

//                float pixel = mapper.grey_image.at<float>(cy, cx);
                float pixel = getColorSubpix(mapper.grey_image, cv::Point2f(cx, cy));
                grey_colors[id] += pixel;

                if (need_color) {
                    cv::Vec3f pixel_c = mapper.color_image.at<cv::Vec3f>(cy, cx);
                    shape->colors[id] += glm::vec4(
                            pixel_c.val[2],
                            pixel_c.val[1],
                            pixel_c.val[0],
                            1.f
                    );
                }

                mapped_count[id]++;
            }
        }
    }
    for (int i=0; i<shape->colors.size(); i++) {
        grey_colors[i] /= mapped_count[i];
    }
    if (need_color) {
        for (int i=0; i<shape->colors.size(); i++) {
            shape->colors[i] /= mapped_count[i];
        }
    }
    delete[]mapped_count;
}

void ColorMapper::optimize_pose_only(GLUnit &u) {
#pragma omp parallel for
    for (int i=0; i<map_units.size(); i++) {
        auto &mapper = map_units[i];
        for (int j=0; j<mapper.vertices_po.size(); j++) {
            auto &verts = mapper.vertices_po[j];
            glm::mat4 transform = mapper.transform_po[j];
            glm::vec4 vert;
            float cx, cy;

            MatrixXf Jg(4, 6), Ju(2, 4), J_Gamma(1, 2), _Jr(1, 6), Jr(verts.size(), 6);
            VectorXf r(verts.size());
            glm::vec4 g;
            int vert_count = 0;

            for (int i = 0; i < verts.size(); i++) {
                GLuint id = verts[i];
                g = transform * glm::vec4(shape->vertices[id], 1.f);
                GLfloat z = .75f + vert.z / 8.f;

                Jg <<
                   0, g.z, -g.y, g.w, 0, 0,
                        -g.z, 0, g.x, 0, g.w, 0,
                        g.y, -g.x, 0, 0, 0, g.w,
                        0, 0, 0, 0, 0, 0;

                cx = g.x * u.f.x / g.z + u.c.x;
                cy = g.y * u.f.y / g.z + u.c.y;

                if (std::isnan(cx) || std::isnan(cy)) {
                    continue;
                }

                if (cx < 3 || cx + 3 > u.GLWidth || cy < 3 || cy + 3 > u.GLHeight) {
                    continue;
                }

                Ju <<
                   u.f.x / g.z, 0, -g.x * u.f.x / g.z / g.z, 0,
                        0, u.f.y / g.z, -g.y * u.f.y / g.z / g.z, 0;

                float pixel = grey_colors[id] - mapper.grey_image.at<float>(cy, cx);

                J_Gamma <<
//                        mapper.grad_x.at<float>(cy, cx),
//                        mapper.grad_y.at<float>(cy, cx);
                        getColorSubpix(mapper.grad_x, cv::Point2f(cx, cy)),
                        getColorSubpix(mapper.grad_y, cv::Point2f(cx, cy));

                _Jr = J_Gamma * Ju * Jg;

                std::cout
//                    << Ju << std::endl
//                    << Jg << std::endl
//                    << J_Gamma << std::endl
//                    << pixel << std::endl
//                    << _Jr << std::endl
                        ;
//            getchar();
                Jr.row(vert_count) = _Jr;
                r(vert_count) = pixel;
                ++vert_count;
            }

            if (!vert_count) continue;

            r = r.topRows(vert_count);
            Jr = Jr.topRows(vert_count);

            MatrixXf src1(6, 6);
            VectorXf src2(6), deltaX(7);
            MatrixXf JrT = Jr.transpose();
            src1 = JrT * Jr;
            src2 = JrT * -r;

            deltaX = src1.ldlt().solve(src2);
//        deltaX = Jr.colPivHouseholderQr().solve(-r);

            std::cout
//                << src1 << std::endl
//                << src2 << std::endl
//                << deltaX << std::endl << std::endl
                    ;
//        getchar();

            float alpha_i = deltaX(0);
            float beta_i = deltaX(1);
            float gamma_i = deltaX(2);
            float ai = deltaX(3);
            float bi = deltaX(4);
            float ci = deltaX(5);

            if (deltaX.norm() > 0.01f) {
                continue;
            }

            bool is_nan = false;
            for (int i=0; i<6; i++) {
                if (std::isnan(deltaX(i))) { is_nan = true; }
            }
            if (is_nan) { continue; }

            glm::mat4 kx(
                    glm::vec4(1.f, gamma_i, -beta_i, 0.f),
                    glm::vec4(-gamma_i, 1.f, alpha_i, 0.f),
                    glm::vec4(beta_i, -alpha_i, 1.f, 0.f),
                    glm::vec4(ai, bi, ci, 1.f)
            );

            mapper.transform_po[j] = kx * mapper.transform_po[j];
        }
    }
}
