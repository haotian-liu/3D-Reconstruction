//
// Created by Haotian on 2017/8/23.
//

#ifndef INC_3DRECONSTRUCTION_RENDERER_H
#define INC_3DRECONSTRUCTION_RENDERER_H

#include <string>
#include <vector>

class Renderer {
public:
    void setName(std::string filename);
    bool samplePly();

private:
    std::string filename;
    std::vector<float> verts;
    std::vector<float> norms;
    std::vector<uint8_t> colors;

    std::vector<uint32_t> faces;
    std::vector<float> uvCoords;
};


#endif //INC_3DRECONSTRUCTION_RENDERER_H
