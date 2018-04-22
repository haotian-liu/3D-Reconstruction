//
// Created by Haotian on 2018/3/22.
//

#include "ColorMapper.h"
#include <fstream>

void ColorMapper::update_config() {
    load_keyframes();
    base_map();
    exit(-1);
}

void ColorMapper::load_keyframes() {
    keys.clear();
    images_grey.clear();
    images_color.clear();

    const std::string path_folder = "/Users/haotian/Downloads/fountain_all/";
    const std::string img_folder = path_folder + "vga/";
    const std::string path_file = "key_convert.txt";
    std::fstream keyFrameFile(path_folder + path_file);

    std::string imagePath;
    while (!keyFrameFile.eof()) {
        keyFrameFile >> imagePath;
        keys.push_back(img_folder + imagePath);
    }

    cv::Mat color;
    cv::Mat grey;

    for (auto &key : keys) {
        color = cv::imread(key);
        cv::cvtColor(color, grey, CV_RGB2GRAY);

        images_color.push_back(color);
        images_grey.push_back(grey);
    }
}

void ColorMapper::base_map() {

}
