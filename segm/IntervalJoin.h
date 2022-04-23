#ifndef INTERVAL_JOIN_H


#include "common.h"
#include "interval_tree.hpp"

using namespace lib_interval_tree;

std::vector<interval<int>> join_intervals(std::vector<std::tuple<int,int,int>> rngs);

#endif

