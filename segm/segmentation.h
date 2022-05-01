#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include "common.h"
#include "interval_tree.hpp"

std::map<int, std::tuple<cv::Rect, int, double>> find_neighbors(
    std::vector<cv::Rect>& glyphs,
    std::vector<std::vector<int>>& nearest_neighbors);

std::map<int, int> group_words(std::vector<cv::Rect>& glyphs);

cv::Rect bounding_rect(std::set<int>& all_inds, std::vector<cv::Rect>& glyphs);

std::vector<std::vector<cv::Rect>> word_limits(
    std::map<int, int>& interword_gaps, std::vector<cv::Rect>& glyphs);

double baseline(std::vector<cv::Rect> glyphs);

std::vector<cv::Rect> group_glyphs(std::vector<cv::Rect>& glyphs);

std::vector<cv::Rect> bounding_rects_for_words(
    std::vector<std::vector<cv::Rect>> words);

#endif
