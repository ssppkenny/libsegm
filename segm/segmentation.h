#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include "common.h"
#include "interval_tree.hpp"
#include "fastcluster.h"

struct words_struct {
    double lower;
    std::vector<std::vector<cv::Rect>> words;
};



enum Position {LEFT = 1, RIGHT, BOTTOM, UP, UNKNOWN};

struct dist_info {
    double distance;
    Position pos;
};


using namespace lib_interval_tree;

double dist(std::tuple<int,int> p1, std::tuple<int,int> p2);

dist_info dist_rects(cv::Rect& r1, cv::Rect& r2);

dist_info distance(std::vector<int>& sc, int pi, std::vector<cv::Rect>& jr);

std::map<int,std::vector<std::vector<int>>> detect_belonging_captions(cv::Mat& mat, std::vector<cv::Rect>& jr, std::vector<int>& pic_inds, std::vector<std::vector<int>> small_components, double most_frequent_height, double multiplier);

void join_with_captions(std::map<int,std::vector<std::vector<int>>>& belongs, std::vector<cv::Rect>& rects, std::vector<cv::Rect>& output );

std::map<int,std::vector<std::vector<int>>> detect_captions(cv::Mat& mat, std::vector<cv::Rect>& joined_rects);

std::vector<words_struct> find_ordered_glyphs(
    std::vector<cv::Rect>& joined_rects);

std::map<int, std::tuple<cv::Rect, int, double>> find_neighbors(
    std::vector<cv::Rect>& glyphs,
    std::vector<std::vector<int>>& nearest_neighbors);

std::map<int, int> group_words(std::vector<cv::Rect>& glyphs);

cv::Rect bounding_rect(std::set<int>& all_inds, std::vector<cv::Rect>& glyphs);

std::vector<std::vector<cv::Rect>> word_limits(
    std::map<int, int>& interword_gaps, std::vector<cv::Rect>& glyphs);

double baseline(std::vector<cv::Rect>& glyphs);

std::vector<cv::Rect> group_glyphs(std::vector<cv::Rect>& glyphs);

std::vector<cv::Rect> bounding_rects_for_words(
    std::vector<std::vector<cv::Rect>> words);

#endif
