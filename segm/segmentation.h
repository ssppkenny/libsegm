#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include "common.h"


std::map<int,std::tuple<cv::Rect,int,double>> find_neighbors(std::vector<cv::Rect>& glyphs, std::vector<std::vector<int>>& nearest_neighbors);


#endif
