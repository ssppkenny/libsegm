#include "segmentation.h"

struct less_than_neighbor {
    inline bool operator()(const std::tuple<cv::Rect, int, double>& r1, const std::tuple<cv::Rect, int, double>& r2) {
        return get<0>(r1).x/(double)std::get<2>(r1) <  get<0>(r2).x/(double)std::get<2>(r2);
    }
};


std::map<int,std::tuple<cv::Rect,int,double>> find_neighbors(std::vector<cv::Rect>& glyphs, std::vector<std::vector<int>>& nearest_neighbors) {
    
    std::map<int,std::tuple<cv::Rect,int,double>> ret_val;

    for (int i=0; i<glyphs.size(); i++) {
        ret_val[i] = std::tuple<cv::Rect,int, double>(cv::Rect(-1,-1,0,0), 0, 0);
    }

    for (int j=0; j<glyphs.size(); j++) {
        cv::Rect r = glyphs[j];
        std::vector<int> nbs = nearest_neighbors[j];
        std::vector<std::tuple<cv::Rect,int, double>> right_nbs;
        for (int i=1;i<nbs.size();i++) {
            cv::Rect nb = glyphs[nbs[i]];
            int a = r.y;
            int b = r.y + r.height;
            int c = nb.y;
            int d = nb.y + nb.height;
            int left = std::max(a,c);
            int right = std::min(b,d);

            if (left < right && nb.x > r.x) {
               right_nbs.push_back(std::make_tuple(nb, nbs[i], (right - left) / (double)nb.height));
            }
        }

        std::sort(right_nbs.begin(), right_nbs.end(), less_than_neighbor());
        if (right_nbs.size() > 0) {
            ret_val[j] = right_nbs[0];
        }
    }

    return ret_val;


}
