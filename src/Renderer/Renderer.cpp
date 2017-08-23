//
// Created by Haotian on 2017/8/23.
//

#include "Renderer.h"
#include <fstream>
#include "tinyply/tinyply.h"

void Renderer::setName(std::string filename) {
    this->filename = filename;
}

typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;
std::chrono::high_resolution_clock c;

inline std::chrono::time_point<std::chrono::high_resolution_clock> now() {
    return c.now();
}

inline double difference_micros(timepoint start, timepoint end) {
    return (double) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

bool Renderer::samplePly() {
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
    timepoint before = now();
    file.read(ss);
    timepoint after = now();

    // Good place to put a breakpoint!
    std::cout << "Parsing took " << difference_micros(before, after) << "μs: " << std::endl;
    std::cout << "\tRead " << verts.size() << " total vertices (" << vertexCount << " properties)." << std::endl;
    std::cout << "\tRead " << norms.size() << " total normals (" << normalCount << " properties)." << std::endl;
    std::cout << "\tRead " << colors.size() << " total vertex colors (" << colorCount << " properties)." << std::endl;
    std::cout << "\tRead " << faces.size() << " total faces (triangles) (" << faceCount << " properties)." << std::endl;
    std::cout << "\tRead " << uvCoords.size() << " total texcoords (" << faceTexcoordCount << " properties)."
              << std::endl;

    return true;
}
