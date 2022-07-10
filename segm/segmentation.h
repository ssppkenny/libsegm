#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include "common.h"
#include "fastcluster.h"
#include "interval_tree.hpp"

struct glyph_result {
    int x;
    int y;
    int width;
    int height;
    int shift;
};

struct words_struct {
    double lower;
    std::vector<std::vector<cv::Rect>> words;
};

struct Word {
    std::vector<glyph_result> glyphs;
    glyph_result bounding_rect;
};

enum Position { LEFT = 1, RIGHT, BOTTOM, UP, UNKNOWN };

struct dist_info {
    double distance;
    Position pos;
};

using namespace lib_interval_tree;

double dist(std::tuple<int, int> p1, std::tuple<int, int> p2);

dist_info dist_rects(cv::Rect& r1, cv::Rect& r2);

dist_info distance(std::vector<int>& sc, int pi, std::vector<cv::Rect>& jr);

std::map<int, std::vector<std::vector<int>>> detect_belonging_captions(
    cv::Mat& mat, std::vector<cv::Rect>& jr, std::vector<int>& pic_inds,
    std::vector<std::vector<int>> small_components, double most_frequent_height,
    double multiplier);

std::vector<int> join_with_captions(std::map<int, std::vector<std::vector<int>>>& belongs,
                        std::vector<cv::Rect>& rects,
                        std::vector<cv::Rect>& output);


std::vector<std::vector<std::tuple<Word, double>>> split_paragraph(std::vector<std::tuple<Word, double>>& tp, int width);

std::vector<std::tuple<Word, double>> transform_paragraph(std::vector<std::vector<std::vector<glyph_result>>>& p, std::vector<double>& indents, int gap_width, int par_num);

std::map<int, std::vector<std::vector<int>>> detect_captions(
    cv::Mat& mat, std::vector<cv::Rect>& joined_rects);

cv::Mat find_reflowed_image(
    std::vector<cv::Rect>& joined_rects, float factor, float zoom_factor, cv::Mat& mat);

std::vector<words_struct> find_ordered_glyphs(
    std::vector<cv::Rect>& joined_rects);

std::map<int, std::tuple<cv::Rect, int, double>> find_neighbors(
    std::vector<cv::Rect>& glyphs,
    std::vector<std::vector<int>>& nearest_neighbors, double average_height);

std::map<int, int> group_words(std::vector<cv::Rect>& glyphs);

cv::Rect bounding_rect(std::set<int>& all_inds, std::vector<cv::Rect>& glyphs);

std::vector<std::vector<cv::Rect>> word_limits(
    std::map<int, int>& interword_gaps, std::vector<cv::Rect>& glyphs);

double baseline(std::vector<cv::Rect>& glyphs);

std::vector<cv::Rect> group_glyphs(std::vector<cv::Rect>& glyphs);

std::vector<cv::Rect> bounding_rects_for_words(
    std::vector<std::vector<cv::Rect>> words);

#endif
