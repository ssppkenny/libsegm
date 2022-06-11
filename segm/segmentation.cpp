#include "segmentation.h"

#include "IntervalJoin.h"
#include "RectJoin.h"

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, int>
    NeighborsGraph;

struct less_than_glyph {
    inline bool operator()(const cv::Rect& r1, const cv::Rect& r2) {
        return r1.x < r2.x;
    }
};
struct less_than_neighbor {
    inline bool operator()(const std::tuple<cv::Rect, int, double>& r1,
                           const std::tuple<cv::Rect, int, double>& r2) {
        return get<0>(r1).x / (double)std::get<2>(r1) <
               get<0>(r2).x / (double)std::get<2>(r2);
    }
};

std::map<int, std::tuple<cv::Rect, int, double>> find_neighbors(
    std::vector<cv::Rect>& glyphs,
    std::vector<std::vector<int>>& nearest_neighbors) {
    std::map<int, std::tuple<cv::Rect, int, double>> ret_val;

    for (int i = 0; i < glyphs.size(); i++) {
        ret_val[i] =
            std::tuple<cv::Rect, int, double>(cv::Rect(-1, -1, 0, 0), 0, 0);
    }

    for (int j = 0; j < glyphs.size(); j++) {
        cv::Rect r = glyphs[j];
        std::vector<int> nbs = nearest_neighbors[j];
        std::vector<std::tuple<cv::Rect, int, double>> right_nbs;
        for (int i = 1; i < nbs.size(); i++) {
            cv::Rect nb = glyphs[nbs[i]];
            int a = r.y;
            int b = r.y + r.height;
            int c = nb.y;
            int d = nb.y + nb.height;
            int left = std::max(a, c);
            int right = std::min(b, d);

            if (left < right && nb.x > r.x) {
                right_nbs.push_back(std::make_tuple(
                    nb, nbs[i], (right - left) / (double)nb.height));
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

std::map<int, int> group_words(std::vector<cv::Rect>& glyphs) {
    double s = 0.0;
    for (int i = 0; i < glyphs.size(); i++) {
        s += glyphs[i].height;
    }
    double av_height = s / glyphs.size();
    std::map<int, int> interword_gaps;

    if (glyphs.size() < 2) {
        return interword_gaps;
    }

    for (int i = 1; i < glyphs.size(); i++) {
        cv::Rect gc = glyphs[i];
        cv::Rect gp = glyphs[i - 1];
        int gap = gc.x - (gp.x + gp.width);
        if (gap > 0.5 * av_height) {
            interword_gaps[i] = gap;
        }
    }
    return interword_gaps;
}

std::vector<std::vector<cv::Rect>> word_limits(
    std::map<int, int>& interword_gaps, std::vector<cv::Rect>& glyphs) {
    std::vector<int> arr;
    for (std::map<int, int>::iterator it = interword_gaps.begin();
         it != interword_gaps.end(); ++it) {
        arr.push_back(it->first);
    }
    std::vector<std::vector<cv::Rect>> words;
    if (arr.size() == 0) {
        std::vector<cv::Rect> copy_of_glyphs;
        std::copy(glyphs.begin(), glyphs.end(),
                  std::back_inserter(copy_of_glyphs));
        words.push_back(copy_of_glyphs);
        return words;
    }

    std::vector<int> limits;
    limits.push_back(arr[0]);

    for (int i = 0; i < arr.size(); i++) {
        if (i != arr.size() - 1) {
            limits.push_back(arr[i + 1] - arr[i]);
        } else {
            limits.push_back(glyphs.size() - arr[i]);
        }
    }

    int total = 0;
    for (int i = 0; i < limits.size(); i++) {
        std::vector<cv::Rect> word;
        for (int j = 0; j < limits[i]; j++) {
            word.push_back(glyphs[total]);
            total++;
        }
        words.push_back(word);
    }
    return words;
}

double baseline(std::vector<cv::Rect>& glyphs) {
    std::vector<int> lowers;
    int min = INT_MAX;
    int max = 0;
    for (int i = 0; i < glyphs.size(); i++) {
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
    std::pair<std::vector<int>, std::vector<float>> p =
        make_hist(lowers, 50, min, max);
    std::vector<int> v = p.first;
    std::vector<float> bin_edges = p.second;
    std::vector<int>::iterator mx =
        std::max_element(v.begin(), v.end());      // [2, 4)
    int argmaxVal = std::distance(v.begin(), mx);  // absolute index of max

    return bin_edges[argmaxVal];
}

std::vector<cv::Rect> group_glyphs(std::vector<cv::Rect>& glyphs) {
    std::map<std::tuple<int, int>, std::vector<int>> numbered_glyphs;
    std::vector<std::tuple<int, int, int>> intervals;
    interval_tree_t<int> tree;
    for (int i = 0; i < glyphs.size(); i++) {
        cv::Rect g = glyphs[i];
        auto it = numbered_glyphs.find(std::make_tuple(g.x, g.x + g.width));
        if (it != numbered_glyphs.end()) {
            std::vector<int> inds =
                numbered_glyphs[std::make_tuple(g.x, g.x + g.width)];
            inds.push_back(i);
            numbered_glyphs[std::make_tuple(g.x, g.x + g.width)] = inds;
        } else {
            std::vector<int> inds;
            inds.push_back(i);
            numbered_glyphs[std::make_tuple(g.x, g.x + g.width)] = inds;
        }
        intervals.push_back(std::make_tuple(g.x, g.x + g.width, i));

        tree.insert(make_safe_interval(g.x, g.x + g.width));
    }

    std::vector<interval<int>> glyph_limits = join_intervals(intervals);
    std::vector<cv::Rect> new_rects;

    for (auto& gl : glyph_limits) {
        std::vector<interval<int>> overlapping_intervals;
        tree.overlap_find_all(
            {gl.low(), gl.high()}, [&overlapping_intervals](auto iter) {
                overlapping_intervals.push_back(
                    make_safe_interval(iter->low(), iter->high()));
                return true;
            });

        std::set<int> all_inds;
        for (auto& intr : overlapping_intervals) {
            std::vector<int> inds =
                numbered_glyphs[std::make_tuple(intr.low(), intr.high())];
            std::copy(inds.begin(), inds.end(),
                      std::inserter(all_inds, all_inds.end()));
        }
        cv::Rect br = bounding_rect(all_inds, glyphs);
        new_rects.push_back(br);
    }
    return new_rects;
}

std::vector<cv::Rect> bounding_rects_for_words(
    std::vector<std::vector<cv::Rect>> words) {
    std::vector<cv::Rect> new_rects;
    for (auto& w : words) {
        std::set<int> all_inds;
        for (int i = 0; i < w.size(); i++) {
            all_inds.insert(i);
        }
        auto br = bounding_rect(all_inds, w);
        new_rects.push_back(br);
    }
    return new_rects;
}

std::vector<words_struct> find_ordered_glyphs(
    std::vector<cv::Rect>& joined_rects) {
    interval_tree_t<int> tree;

    int size = joined_rects.size();
    double data[size][2];

    for (int i = 0; i < size; i++) {
        data[i][0] = joined_rects[i].x + joined_rects[i].width / 2.0;
        data[i][1] = joined_rects[i].y + joined_rects[i].height / 2.0;
    }

    ::flann::Matrix<double> dataset(&data[0][0], size, 2);

    int k = std::min(100, size);

    ::flann::Index<::flann::L2<double>> index(dataset,
                                              ::flann::KDTreeIndexParams(1));
    index.buildIndex();

    ::flann::Matrix<int> indices(new int[size * k], size, k);
    ::flann::Matrix<double> dists(new double[size * k], size, k);

    double q[size][2];

    for (int i = 0; i < size; i++) {
        q[i][0] = joined_rects[i].x + joined_rects[i].width / 2.0;
        q[i][1] = joined_rects[i].y + joined_rects[i].height / 2.0;
    }

    ::flann::Matrix<double> query(&q[0][0], size, 2);
    index.knnSearch(query, indices, dists, k, ::flann::SearchParams());

    std::vector<std::vector<int>> nearest_neighbors;
    for (int i = 0; i < size; i++) {
        std::vector<int> nbs;
        for (int j = 0; j < k; j++) {
            nbs.push_back(indices[i][j]);
        }
        nearest_neighbors.push_back(nbs);
    }
    std::map<int, std::tuple<cv::Rect, int, double>> neighbors =
        find_neighbors(joined_rects, nearest_neighbors);

    NeighborsGraph g;

    for (auto it = neighbors.begin(); it != neighbors.end(); it++) {
        auto k = it->first;
        auto v = it->second;

        if (get<0>(v).x == -1) {
            add_vertex(k, g);
        } else {
            add_edge(k, std::get<1>(v), g);
        }
    }

    std::vector<int> c(num_vertices(g));

    connected_components(
        g, make_iterator_property_map(c.begin(), get(vertex_index, g), c[0]));

    std::map<int, std::vector<int>> cc;
    for (int i = 0; i < c.size(); i++) {
        if (cc.find(c[i]) == cc.end()) {
            std::vector<int> v;
            v.push_back(i);
            cc[c[i]] = v;
        } else {
            auto v = cc[c[i]];
            v.push_back(i);
            cc[c[i]] = v;
        }
    }

    std::set<int> map_keys;

    for (auto it = cc.begin(); it != cc.end(); ++it) {
        map_keys.insert(it->first);
    }

    std::vector<std::tuple<int, int, int>> line_limits;
    std::set<int>::iterator it = map_keys.begin();
    // int ind = 0;
    while (it != map_keys.end()) {
        int k = (*it);
        std::vector<int> v = cc[k];

        int min1 = INT_MAX;
        int max1 = 0;
        for (auto e : v) {
            cv::Rect r = joined_rects[e];
            if (r.y < min1) {
                min1 = r.y;
            }
            if (r.y + r.height > max1) {
                max1 = r.y + r.height;
            }
        }
        line_limits.push_back(std::make_tuple(min1, max1, k));
        // ind++;
        it++;
    }

    std::vector<interval<int>> joined_lined_limits =
        join_intervals(line_limits);

    for (int i = 0; i < joined_lined_limits.size(); i++) {
        tree.insert(make_safe_interval(joined_lined_limits[i].low(),
                                       joined_lined_limits[i].high()));
    }

    std::map<std::tuple<int, int>, std::vector<cv::Rect>> lines;

    std::set<std::tuple<int, int>> lines_keys;

    for (int i = 0; i < joined_rects.size(); i++) {
        cv::Rect r = joined_rects[i];
        std::vector<interval<int>> rt;
        tree.overlap_find_all({r.y, r.y}, [&rt](auto iter) {
            rt.push_back(make_safe_interval(iter->low(), iter->high()));
            return true;
        });

        if (rt.size() > 0) {
            if (lines.find(std::make_tuple(rt[0].low(), rt[0].high())) !=
                lines.end()) {
                std::vector<cv::Rect> v =
                    lines[std::make_tuple(rt[0].low(), rt[0].high())];
                v.push_back(r);
                lines[std::make_tuple(rt[0].low(), rt[0].high())] = v;
            } else {
                lines_keys.insert(std::make_tuple(rt[0].low(), rt[0].high()));
                std::vector<cv::Rect> v;
                v.push_back(r);
                lines[std::make_tuple(rt[0].low(), rt[0].high())] = v;
            }
        }
    }

    std::vector<words_struct> lines_of_words;

    for (auto it = lines_keys.begin(); it != lines_keys.end(); it++) {
        auto k = (*it);
        auto v = lines[k];
        auto grouped_glyphs = group_glyphs(v);
        double lower = baseline(grouped_glyphs);
        std::sort(grouped_glyphs.begin(), grouped_glyphs.end(),
                  less_than_glyph());
        auto interword_gaps = group_words(grouped_glyphs);
        std::vector<std::vector<cv::Rect>> words =
            word_limits(interword_gaps, grouped_glyphs);
        struct words_struct ws = {lower, words};
        lines_of_words.push_back(ws);
    }

    return lines_of_words;
}

double dist(std::tuple<int,int> p1, std::tuple<int,int> p2) {
    int x1 = std::get<0>(p1);
    int y1 = std::get<1>(p1);
    int x2 = std::get<0>(p2);
    int y2 = std::get<1>(p2);
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));    
}

dist_info dist_rects(cv::Rect& r1, cv::Rect& r2) {
    Position position = Position::UNKNOWN;
    int ab = r1.x;
    int ae = r1.x + r1.width;
    int bb = r2.x;
    int be = r2.x + r2.width;
    int cb = r1.y;
    int ce = r1.y + r1.height;
    int db = r2.y;
    int de = r2.y + r2.height;

    bool no_x_overlap = bb > ae || ab > be;
    bool no_y_overlap = db > ce || cb > de;

    std::vector<double> distances;

    if (no_x_overlap && no_y_overlap) {
        std::vector<std::tuple<int,int>> points1 {std::make_tuple(r1.x, r1.y), std::make_tuple(r1.x, r1.y + r1.height), std::make_tuple(r1.x + r1.width, r1.y), std::make_tuple(r1.x + r1.width, r1.y + r1.height)};        
        std::vector<std::tuple<int,int>> points2 {std::make_tuple(r2.x, r2.y), std::make_tuple(r2.x, r2.y + r2.height), std::make_tuple(r2.x + r2.width, r2.y), std::make_tuple(r2.x + r2.width, r2.y + r2.height)};

        for (auto p: points1) {
            for (auto q : points2) {
                distances.push_back(dist(p,q));
            }
        }

        auto min_value = *std::min_element(distances.begin(),distances.end());
        dist_info di = {min_value, position};
        return di;
    } 
    if (no_x_overlap && !no_y_overlap) {
        int x1 = r1.x;
        int x2 = r1.x + r1.width;
        int x3 = r2.x;
        int x4 = r2.x + r2.width;
        position = r1.x < r2.x ? Position::LEFT : Position::RIGHT;
        std::vector<int> distances {std::abs(x1-x3), std::abs(x1-x4), std::abs(x2-x3), std::abs(x2 -x4)};
        auto min_value = *std::min_element(distances.begin(), distances.end());
        dist_info di = {(double)min_value, position};
        return di;
    }

    if (!no_x_overlap && no_y_overlap) {
        int x1 = r1.y;
        int x2 = r1.y + r1.height;
        int x3 = r2.y;
        int x4 = r2.y + r2.height;
        position = r1.y < r2.y ? Position::UP : Position::BOTTOM;
        std::vector<int> distances {std::abs(x1-x3), std::abs(x1-x4), std::abs(x2-x3), std::abs(x2 -x4)};
        auto min_value = *std::min_element(distances.begin(), distances.end());
        dist_info di = {(double)min_value, position};
        return di;
    }

    if (!no_x_overlap && !no_y_overlap) {
        dist_info di = {0, position};
        return di;
    }

    dist_info di = {0, position};
    return di;


}

dist_info distance(std::vector<int>& sc, int pi, std::vector<cv::Rect>& jr) {
    std::vector<cv::Rect> rects;
    for (int i=0;i<sc.size();i++) {
        int n = sc[i];
        cv::Rect r = jr[n];
        rects.push_back(r);
    }

    std::set<int> inds;
    for (int i=0;i<rects.size(); i++) {
        inds.insert(i);
    }

    cv::Rect br = bounding_rect(inds, rects);
    cv::Rect pic = jr[pi];
    return dist_rects(br, pic);

}

std::map<int,std::vector<std::vector<int>>> detect_belonging_captions(cv::Mat& mat, std::vector<cv::Rect>& jr, std::vector<int>& pic_inds, std::vector<std::vector<int>> small_components, double most_frequent_height, double multiplier) {
    std::map<int, std::vector<std::vector<int>>> belongs;
    for (int k=0; k<small_components.size(); k++) {
        std::vector<std::tuple<double,int,Position>> ds;
        std::vector<int> sc = small_components[k];
        for (int j=0;j<pic_inds.size(); j++) {
            int pi = pic_inds[j];
            dist_info di = distance(sc, pi, jr);
            if (di.pos != Position::UNKNOWN) {
                // add check if there is nothing aroud this component
				// create bounding rect for small components
				
				std::set<int> rect_inds;
				std::vector<cv::Rect> to_join;
				for (int l=0;l<sc.size();l++) {
					rect_inds.insert(l);
					to_join.push_back(jr[sc[l]]);
				}

				cv::Rect br = bounding_rect(rect_inds, to_join);
				


				int width = mat.cols;
				int height = mat.rows;
				int left = br.x - int(most_frequent_height) > 0 ? br.x - int(most_frequent_height) : 0;
                int right = br.x + br.width + int(most_frequent_height) < width ?  br.x + br.width + int(most_frequent_height) : width - 1;

				int upper = br.y - int(most_frequent_height) > 0 ? br.y - int(most_frequent_height) : 0;
                int lower = br.y + br.height + int(most_frequent_height) < height ?  br.y + br.height + int(most_frequent_height) : height - 1;

				int h = lower - upper;
				int w = right - left;
				cv::Rect outer = cv::Rect(left, upper, right - left, lower - upper);
				cv::Mat m = mat(outer);

				int outer_sum = cv::countNonZero(mat(outer));
				int inner_sum = cv::countNonZero(mat(br));
                int area = 2*(lower-upper)*most_frequent_height + 2*(right-left)*most_frequent_height - 4*most_frequent_height*most_frequent_height;
				double ratio = (outer_sum - inner_sum)/(double)area;
			
				bool is_caption = (ratio < 0.1);
				//bool is_caption = (left_sum + upper_sum + right_sum < 10) || (upper_sum + right_sum + lower_sum < 10) || (right_sum + lower_sum + left_sum < 10) || (lower_sum + left_sum + upper_sum < 10);

				if (is_caption && br.height < multiplier * most_frequent_height) {
					ds.push_back(std::make_tuple(di.distance, pi, di.pos));
				}

            }
        }
        if (ds.size() > 0) {
            std::sort(ds.begin(), ds.end());
            std::tuple<double,int,Position> t = ds[0];
            int pos = std::get<1>(t);
            if (std::get<0>(t) < multiplier * most_frequent_height) {
                if (belongs.find(pos) == belongs.end()) {
                    std::vector<std::vector<int>> v;
                    v.push_back(sc);
                    belongs[pos] = v;
                } else {
                    std::vector<std::vector<int>> v = belongs[pos];
                    v.push_back(sc);
                    belongs[pos] = v;
                }
            }
        }
    }
    return belongs;
}

void join_with_captions(std::map<int,std::vector<std::vector<int>>>& belongs, std::vector<cv::Rect>& rects, std::vector<cv::Rect>& output ) {

    std::vector<int> belong_keys;
    for(auto const& itmap: belongs) {
        belong_keys.push_back(itmap.first);
    }

    std::vector<int> inds_to_ignore;
    for (auto key : belong_keys) {
        inds_to_ignore.push_back(key);
        std::vector<std::vector<int>> small_components = belongs[key];
        std::vector<cv::Rect> to_join;
        to_join.push_back(rects[key]);
        for (auto sc : small_components) {
            for (auto ind : sc) {
                inds_to_ignore.push_back(ind);
                to_join.push_back(rects[ind]);
            }
        }
        std::set<int> rect_inds;
        for (int l=0;l<to_join.size();l++) {
            rect_inds.insert(l);
        }


        cv::Rect br = bounding_rect(rect_inds, to_join);
        output.push_back(br);

    }

    for (int i=0;i<rects.size(); i++) {
        if (std::find(inds_to_ignore.begin(), inds_to_ignore.end(), i) == inds_to_ignore.end()) {
            output.push_back(rects[i]);
        }
    }

	output = join_rects(output);
}

std::map<int,std::vector<std::vector<int>>> detect_captions(cv::Mat& mat, std::vector<cv::Rect>& joined_rects) {
    std::map<int,std::vector<std::vector<int>>> belongs;
    double s = 0.0;
    int n = joined_rects.size();
    std::vector<int> pic_inds;
    for (int i=0;i<n;i++) {
        cv::Rect jr = joined_rects[i];
        s += jr.height;
    }

    double most_frequent_height = s / joined_rects.size();
    for (int i=0;i<n;i++) {
        cv::Rect jr = joined_rects[i];
        if (jr.height > 7*most_frequent_height) {
            pic_inds.push_back(i);
        }
    }
    double* distmat = new double[(n*(n-1))/2];
    int i, k, j;
    for (i=k=0; i<n; i++) {
        for (j=i+1; j<n; j++) {
        // compute distance between observables i and j  
        double x1 = (joined_rects[i].x + joined_rects[i].width)/2;
        double y1 = (joined_rects[i].y + joined_rects[i].height)/2;
        double x2 = (joined_rects[j].x + joined_rects[j].width)/2;
        double y2 = (joined_rects[j].y + joined_rects[j].height)/2;
        distmat[k] = sqrt( (x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2) );
        k++;
        }
    }
    int* merge = new int[2*(n-1)];
    double* height = new double[n-1];
    hclust_fast(n, distmat, HCLUST_METHOD_SINGLE, merge, height);


    int* labels = new int[n];

    int last_cluster_count = std::numeric_limits<int>::max();   
    double x = 0.5;
    while (x<=4) {
        double coef = x;
        x+=0.1;
        cutree_cdist(n, merge, height, most_frequent_height * coef, labels);

        std::map<int,std::vector<int>> res;
        std::set<int> keys;

        for (int t=0;t<n;t++) {
            int key = labels[t];
            keys.insert(key);
            if (res.find(key) != res.end()) {
                std::vector<int> v = res[key];
                v.push_back(t);
                res[key] = v;
            } else {
                std::vector<int> v;
                v.push_back(t);
                res[key] = v;
            }
        }

        std::vector<std::vector<int>> small_components;

        if (keys.size() == last_cluster_count) {


            for (auto key : keys) {
                std::vector<int> sc = res[key];
                if (sc.size() > 0 && sc.size() < 100) {
                    for (auto pi : pic_inds) {
                        sc.erase(std::remove(sc.begin(), sc.end(), pi), sc.end());
                    }
                    small_components.push_back(sc);
                }
            }

           belongs = detect_belonging_captions(mat, joined_rects, pic_inds, small_components, most_frequent_height, coef*3); 
           break;
        }

        last_cluster_count = keys.size();


    }



    delete[] distmat;
    delete[] merge;
    delete[] height;
    delete[] labels;
    return belongs;
}
