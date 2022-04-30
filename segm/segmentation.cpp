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


cv::Rect bounding_rect(std::set<int>& all_inds, std::vector<cv::Rect>& glyphs) {
    int minx = INT_MAX;
    int maxx = 0;
    int miny = INT_MAX;
    int maxy = 0;

    for (auto ind : all_inds) {
        cv::Rect g = glyphs[ind];
        if (g.x < minx) {
            minx = g.x;
        }
        if (g.x + g.width > maxx) {
            maxx = g.x + g.width;
        }
        if (g.y < miny) {
            miny = g.y;
        }
        if (g.y + g.height > maxy) {
            maxy = g.y + g.height;
        }
    }

    return cv::Rect(minx, miny, maxx - minx, maxy - miny);
}

std::map<int,int> group_words(std::vector<cv::Rect>& glyphs) {
    double s = 0.0;
    for (int i=0;i<glyphs.size(); i++) {
        s += glyphs[i].height;
    }
    double av_height = s / glyphs.size();
    std::map<int,int> interword_gaps;

    if (glyphs.size() < 2) {
        return interword_gaps;    
    }

    for (int i=1;i<glyphs.size(); i++) {
        cv::Rect gc = glyphs[i];
        cv::Rect gp = glyphs[i-1];
        int gap = gc.x - (gp.x + gp.width);
        if (gap > 0.5 * av_height) {
            interword_gaps[i] = gap;
        } 
    }
    return interword_gaps;

}


std::vector<std::vector<cv::Rect>> word_limits(std::map<int,int>& interword_gaps, std::vector<cv::Rect>& glyphs) {
    std::vector<int> arr;
    for(std::map<int,int>::iterator it = interword_gaps.begin(); it != interword_gaps.end(); ++it) {
      arr.push_back(it->first);
    }
    std::vector<std::vector<cv::Rect>> words;
    if (arr.size() == 0) {
        std::vector<cv::Rect> copy_of_glyphs;
        std::copy(glyphs.begin(), glyphs.end(), std::back_inserter(copy_of_glyphs));
        words.push_back(copy_of_glyphs);
        return words;
    }

    std::vector<int> limits;
    limits.push_back(arr[0]);

    for (int i=0; i<arr.size(); i++) {
        if (i != arr.size() - 1) {
            limits.push_back(arr[i+1] - arr[i]);
        } else {
            limits.push_back(glyphs.size() - arr[i]);
        }
    }
    
    int total = 0;
    for (int i=0; i<limits.size(); i++) {
        std::vector<cv::Rect> word;
        for (int j=0; j<limits[i]; j++) {
            word.push_back(glyphs[total]);
            total++;
        }
        words.push_back(word);
    }
    return words;

}

double baseline(std::vector<cv::Rect> glyphs) {
    std::vector<int> lowers;
    int min = INT_MAX;
    int max = 0;
    for (int i=0;i<glyphs.size(); i++) {
        cv::Rect g = glyphs[i]; 
        int lower = g.y + g.height;
        lowers.push_back(lower);
        if (lower < min) {
            min = lower;
        }
        if (lower > max) {
            max = lower;
        }
        
    }
    std::pair<std::vector<int>, std::vector<float>> p = make_hist(lowers, 50, min, max);
    std::vector<int> v = p.first;
    std::vector<float> bin_edges = p.second;
    std::vector<int>::iterator mx = std::max_element(v.begin(), v.end()); // [2, 4)
    int argmaxVal = std::distance(v.begin(), mx); // absolute index of max

    return bin_edges[argmaxVal];

}
