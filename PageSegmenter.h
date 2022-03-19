//
// Created by Sergey Mikhno on 2019-05-13.
//

#ifndef FLOW_READER_PAGESEGMENTER_H
#define FLOW_READER_PAGESEGMENTER_H

#include "common.h"


typedef std::tuple<double,double> double_pair ;
typedef boost::adjacency_list < boost::vecS, boost::vecS, boost::undirectedS, double_pair > Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;


struct line_limit {
    line_limit(int upper, int upper_baseline, int lower_baseline, int lower) {
        this->upper = upper;
        this->upper_baseline = upper_baseline;
        this->lower_baseline = lower_baseline;
        this->lower = lower;


    }
    int upper, upper_baseline, lower, lower_baseline;
};


struct cc_result {
    std::vector<double_pair> centers;
    double  average_hight;
    bool whole_page;
};

struct PairXOrder {
    bool operator()(double_pair const &lhs, double_pair const &rhs) const {
        return get<0>(lhs) < get<0>(rhs);
    }
};

struct SortLineLimits {
    bool operator()(line_limit const &lhs, line_limit const &rhs) const {
        return lhs.lower_baseline < rhs.lower_baseline;
    }

};



class PageSegmenter {

    public:
        PageSegmenter(cv::Mat& mat) {
            this->mat = mat;
        }

        std::vector<glyph> get_glyphs();
    
    virtual ~PageSegmenter() {
        g.clear();
    }

    private:
        cv::Mat mat;
        std::map<double_pair,int> center_numbers;
        std::map<double_pair,cv::Rect> rd;
        Graph g;
        int line_height = 0;
        std::vector<line_limit> get_line_limits();
        std::map<int,int> calculate_left_indents(std::vector<int> lefts);
        cc_result get_cc_results();
        std::vector<line_limit> join_lines(std::vector<line_limit> line_limits);
        std::map<int,std::vector<double_pair>> get_connected_components(std::vector<double_pair>& center_list, double averahe_hight);
        line_limit find_baselines(std::vector<double_pair>& cc);
        
        
};


#endif //FLOW_READER_PAGESEGMENTER_H
