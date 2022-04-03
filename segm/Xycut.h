//
// Created by sergey on 01.09.19.
//

#ifndef FLOW_READER_XYCUT_H
#define FLOW_READER_XYCUT_H

#include "ImageNode.h"
#include <opencv2/opencv.hpp>

struct Output
{
    cv::Mat image;
    int x;
    int y;
};

class Xycut
{

public:
    Xycut(cv::Mat image)
    {
        cv::Mat clone = image.clone();
        //cv::cvtColor(clone, clone, cv::COLOR_BGR2GRAY);
        this->image = clone;
    }
    std::vector<ImageNode> xycut();
private:
    cv::Mat image;
    Output image_without_white_borders(cv::Mat& img);
    void xycut_vertical_find(cv::Mat img, double threshold, int level, int height, int width, std::vector<int>& lst);
    void xycut_horizontal_find(cv::Mat img, double threshold, int level, int height, int width, std::vector<int>& lst);
    void xycut_vertical_cut(cv::Mat img, double threshold, ImageNode* tree, int level, int height, int width);
    void xycut_horizontal_cut(cv::Mat img, double threshold, ImageNode* tree, int level, int height, int width);


};


#endif //FLOW_READER_XYCUT_H
