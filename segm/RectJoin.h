#ifndef RectJoin_h

#include "common.h"
#include "interval_tree.hpp"

enum sweep_type
{
    Left = 0,
    Right = 1
};

struct sweep_event
{
    int x;
    cv::Rect rect;
    sweep_type type;
};

using namespace lib_interval_tree;

std::vector<cv::Rect> join_rects(std::vector<Rect>& rects);

std::vector<std::pair<interval<int>,interval<int>>> all_pairs(std::vector<interval<int>> intervals);

#endif
