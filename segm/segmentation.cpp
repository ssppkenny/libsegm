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

std::tuple<std::vector<words_struct>,double> search_lines(std::vector<cv::Rect>& joined_rects, float factor, float zoom_factor, cv::Mat& mat) {

    interval_tree_t<int> tree;

    int size = joined_rects.size();


    float data[size][2];

    double sum_of_heights = 0.0;

    for (int i = 0; i < size; i++) {
        data[i][0] = joined_rects[i].x + joined_rects[i].width / 2.0;
        data[i][1] = joined_rects[i].y + joined_rects[i].height / 2.0;
        sum_of_heights += joined_rects[i].height;
    }

    double average_height = sum_of_heights / size;

    ::flann::Matrix<float> dataset(&data[0][0], size, 2);

    int k = std::min(20, size);

    ::flann::Index<::flann::L2<float>> index(dataset,
                                              ::flann::KDTreeIndexParams());
    index.buildIndex();

    ::flann::Matrix<int> indices(new int[size * k], size, k);
    ::flann::Matrix<float> dists(new float[size * k], size, k);

    float q[size][2];

    for (int i = 0; i < size; i++) {
        q[i][0] = joined_rects[i].x + joined_rects[i].width / 2.0;
        q[i][1] = joined_rects[i].y + joined_rects[i].height / 2.0;
    }

    ::flann::Matrix<float> query(&q[0][0], size, 2);
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
        find_neighbors(joined_rects, nearest_neighbors, average_height);

    NeighborsGraph g;

    for (auto it = neighbors.begin(); it != neighbors.end(); it++) {
        auto k = it->first;
        auto v = it->second;

        if (get<0>(v).x == -1) {
            add_edge(k, k, g);
        } else {
            add_edge(k, std::get<1>(v), g);
        }
    }

    int g_num_vertices = num_vertices(g);
    std::vector<int> c(g_num_vertices);

    assert(joined_rects_size == g_num_vertices);

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

    return std::make_tuple(lines_of_words, average_height);
}


std::vector<int> find_paragraphs(std::vector<int> lefts, double average_height) {
    int n = lefts.size();
    std::vector<int> ret_val;
    for (int i = 0; i<n; i++) {
        int l = lefts[i];
        if (i > 0 && i < n - 1) {
            if (l - lefts[i+1] > average_height && l - lefts[i-1] > average_height) {
                ret_val.push_back(i);
            }
        }
        else if (i == 0) {
            if (l - lefts[i+1] > average_height) {
                ret_val.push_back(i);
            }
        } else if (i == n - 1) {
            if (l - lefts[i-1] > average_height) {
                ret_val.push_back(i);
            }
        }
    }
    return ret_val;
}


std::vector<std::vector<std::vector<glyph_result>>> transform(std::vector<std::vector<std::vector<glyph_result>>> lines_of_words, float zoom_factor) {
    std::vector<std::vector<std::vector<glyph_result>>> ret_val;
    for (int i=0; i<lines_of_words.size(); i++)
    {
        std::vector<std::vector<glyph_result>> line = lines_of_words[i];
        std::vector<std::vector<glyph_result>> w;;
        
        for (int j=0; j<line.size(); j++)
        {
            std::vector<glyph_result> word = line[j];
            std::vector<glyph_result> w1;
            for (int n=0; n<word.size(); n++)
            {
                glyph_result r = word[n];
                glyph_result res;
                res.x = (int)(r.x * zoom_factor);
                res.y = (int)(r.y * zoom_factor);
                res.width = (int)(r.width * zoom_factor);
                res.height = (int)(r.height * zoom_factor);
                res.shift = (int)(r.shift * zoom_factor);
                w1.push_back(res);

            }
            w.push_back(w1);
        }
        ret_val.push_back(w);
    }

    return ret_val;
    
}


double findMedian(vector<int> a)
{
    int n = a.size();
    // If size of the arr[] is even
    if (n % 2 == 0) {
  
        // Applying nth_element
        // on n/2th index
        nth_element(a.begin(),
                    a.begin() + n / 2,
                    a.end());
  
        // Applying nth_element
        // on (n-1)/2 th index
        nth_element(a.begin(),
                    a.begin() + (n - 1) / 2,
                    a.end());
  
        // Find the average of value at
        // index N/2 and (N-1)/2
        return (double)(a[(n - 1) / 2]
                        + a[n / 2])
               / 2.0;
    }
  
    // If size of the arr[] is odd
    else {
  
        // Applying nth_element
        // on n/2
        nth_element(a.begin(),
                    a.begin() + n / 2,
                    a.end());
  
        // Value at index (N/2)th
        // is the median
        return (double)a[n / 2];
    }
}


std::map<int, std::tuple<cv::Rect, int, double>> find_neighbors(
    std::vector<cv::Rect>& glyphs,
    std::vector<std::vector<int>>& nearest_neighbors, double average_height) {
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
            /* int b = r.height >= (average_height / 5.0) ? r.y + r.height : r.y + 2*r.height; */
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
        } else {
                for (int i = 1; i < nbs.size(); i++) {
                    cv::Rect nb = glyphs[nbs[i]];
                    int a = r.y;
                    int b = r.y + r.height;
                    int c = nb.y;
                    int d = nb.y + nb.height;
                    int left = std::max(a, c);
                    int right = std::min(b, d);

                    if (left < right && nb.x < r.x && abs(nb.x - r.x) < average_height)  {
                        right_nbs.push_back(std::make_tuple(
                            nb, nbs[i], (right - left) / (double)nb.height));
                    }
                }
                
            std::sort(right_nbs.begin(), right_nbs.end(), less_than_neighbor());
            if (right_nbs.size() > 0) {
                ret_val[j] = right_nbs[0];
            } else {
                for (int i = 1; i < nbs.size(); i++) {
                    cv::Rect nb = glyphs[nbs[i]];
                    int a = r.y;
                    int b = r.y + 2*r.height;
                    int c = nb.y;
                    int d = nb.y + nb.height;

                    int left = std::max(a, c);
                    int right = std::min(b, d);

                    if (left < right && r.x > nb.x  && r.x + r.width < nb.x + nb.width && nb.x - r.x < 0.5*average_height && r.height < 0.4*average_height )  {
                        right_nbs.push_back(std::make_tuple(
                            nb, nbs[i], (right - left) / (double)nb.height));
                    }
                }

                std::sort(right_nbs.begin(), right_nbs.end(), less_than_neighbor());
                if (right_nbs.size() > 0) {
                    ret_val[j] = right_nbs[0];
                }    
            }
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


std::vector<std::tuple<Word, double>> transform_paragraph(std::vector<std::vector<std::vector<glyph_result>>>& p, std::vector<double>& indents, int gap_width, int par_num) {
    std::vector<std::tuple<Word, double>> ret_val;
    
    std::vector<std::vector<Word>> new_p;

    for (auto line : p) {
        std::vector<Word> new_words;
        for (auto w : line) {
            std::set<int> inds;
            std::vector<cv::Rect> word;
            for (int i=0; i<w.size(); i++) {
                inds.insert(i);
                cv::Rect r(w[i].x, w[i].y, w[i].width, w[i].height);
                word.push_back(r);
            }
            cv::Rect rect = bounding_rect(inds, word);
            glyph_result glyph_rect = {rect.x, rect.y, rect.width, rect.height, 0};
            Word _word = {w, glyph_rect};
            new_words.push_back(_word);
        }
        new_p.push_back(new_words);
    }

    std::vector<std::vector<std::tuple<Word, double>>> new_lines;
    
    std::vector<glyph_result> _w;
    glyph_result _g = {0,0,0,0,0};
    _w.push_back(_g);
    Word w = {_w, _g};

    for (int i=0; i<new_p.size(); i++) {
        std::vector<std::tuple<Word, double>> new_line;
        if (i==0) {
            new_line.insert(new_line.begin(), std::make_tuple(w, indents[par_num]));
        }

        std::vector<Word> c;
        for (auto t : new_p[i]) {
            c.push_back(t);
        }

        for (int k=0; k<c.size(); k++) {
            Word x = c[k];
            if (k== new_p[i].size() - 1) {
                new_line.push_back(std::make_tuple(x, gap_width));
            } else {
                double gap = new_p[i][k+1].bounding_rect.x - (new_p[i][k].bounding_rect.x + new_p[i][k].bounding_rect.width);
                new_line.push_back(std::make_tuple(x, gap));
            }
        }
        new_line.push_back(std::make_tuple(w, gap_width));
        new_lines.push_back(new_line);
        

    }

    for (auto w : new_lines) {
        for (auto nl: w) {
            ret_val.push_back(nl);
        }
    }


    return ret_val;
}


std::vector<std::vector<std::tuple<Word, double>>> split_paragraph(std::vector<std::tuple<Word, double>>& words, int width) {
    int act_width = 0;
    std::vector<std::vector<std::tuple<Word, double>>> new_lines;
    std::vector<std::tuple<Word, double>> line_words;

    for (auto word : words) {
        Word w = std::get<0>(word);
        auto br = w.bounding_rect;
        int g = std::get<1>(word);
        act_width += w.bounding_rect.width;
        if (act_width > width) {
            std::vector<std::tuple<Word, double>> copy_line_words;
            for (auto x : line_words) {
                copy_line_words.push_back(x);
            }
            new_lines.push_back(copy_line_words);
            line_words = std::vector<std::tuple<Word, double>>();
            line_words.push_back(std::make_tuple(w, g));
            act_width = w.bounding_rect.width + g;
        } else {
            line_words.push_back(std::make_tuple(w, g));
            act_width += g;
        }

    }

    if (line_words.size() > 0) {
        new_lines.push_back(line_words);
    }

    return new_lines;


}

cv::Mat find_reflowed_image(
    std::vector<cv::Rect>& joined_rects, float factor, float zoom_factor, cv::Mat& mat) {
    
    std::vector<words_struct> lines_of_words;
    double average_height = 0.0;
    std::vector<int> line_sizes;
    std::tuple<std::vector<words_struct>,double> tup = search_lines(joined_rects, factor, zoom_factor, mat);
    average_height = std::get<1>(tup);
    lines_of_words = std::get<0>(tup);

    // begin reflow
    cv::Mat img;
    cv::Size s = mat.size();
    int rows = (int)(s.height * zoom_factor);
    int cols = (int)(s.width * zoom_factor);
    cv::resize(mat, img, Size(cols, rows), cv::INTER_LINEAR);
    int new_width = s.width * factor;
    int margin = (int)(0.05 * zoom_factor * new_width);
    std::vector<std::vector<std::vector<glyph_result>>> out;
    std::vector<glyph_result> p;
    double sum = 0;

    std::vector<int> lefts;
    std::vector<int> heights;
    int max_height = -1;
    for (int i=0; i<lines_of_words.size(); i++)
    {
        words_struct words_s = lines_of_words[i];
        double lower = words_s.lower;
        std::vector<std::vector<cv::Rect>> words = words_s.words;
        std::vector<std::vector<glyph_result>> w;
        
        int left = INT_MAX;
        
        for (int j=0; j<words.size(); j++)
        {
            std::vector<cv::Rect> word = words[j];
            std::vector<glyph_result> w1;
            for (int n=0; n<word.size(); n++)
            {
                cv::Rect r = word[n];
                glyph_result res;
                res.x = r.x;
                res.y = r.y;
                res.width = r.width;
                res.height = r.height;
                res.shift = (int)lower - (r.y + r.height);
                w1.push_back(res);
                p.push_back(res);
                sum += (res.width * zoom_factor);
                if (res.x < left) {
                    left = res.x;
                }
                if (res.height > max_height) {
                    max_height = res.height;
                }

            }
            w.push_back(w1);
        }
        lefts.push_back((int)(left));
        heights.push_back((int)(zoom_factor * max_height));
        out.push_back(w);
    }


    // cal average width;
    double av_width = sum / p.size();
    out = transform(out, zoom_factor);

    std::vector<int> par_nums = find_paragraphs(lefts, average_height);


    if (par_nums.size() == 0 || par_nums[0] != 0) {
        par_nums.insert(par_nums.begin(), 0);
    }


    int left = *min_element(lefts.begin(), lefts.end());

    int from_ = 0;
    std::vector<std::vector<std::vector<std::vector<glyph_result>>>> paragraphs;
    for (int i=1; i<out.size(); i++) {
        if (std::find(par_nums.begin(), par_nums.end(), i) != par_nums.end()) {
            std::vector<std::vector<std::vector<glyph_result>>>::const_iterator first = out.begin() + from_;
            std::vector<std::vector<std::vector<glyph_result>>>::const_iterator second = out.begin() + i;
            std::vector<std::vector<std::vector<glyph_result>>> sub_vector(first, second);
            from_ = i;
            paragraphs.push_back(sub_vector);
        }
    }


    std::vector<std::vector<std::vector<glyph_result>>>::const_iterator first = out.begin() + par_nums[par_nums.size()-1];
    std::vector<std::vector<std::vector<glyph_result>>>::const_iterator second = out.end();
    std::vector<std::vector<std::vector<glyph_result>>> sub_vector(first, second);
    paragraphs.push_back(sub_vector);


    std::vector<double> indents;

    for (int i=0; i<par_nums.size(); i++) {
        indents.push_back(av_width * 2); 
    }


    std::vector<std::vector<std::tuple<Word, double>>> transform_paragraphs;

    for (int i=0; i<paragraphs.size(); i++) {
       auto p = paragraphs[i];
       auto tp = transform_paragraph(p, indents, (int)(av_width), std::min(i, (int)(indents.size() - 1)));
       transform_paragraphs.push_back(tp);
    }

    std::vector<std::vector<std::tuple<Word, double>>> stps;
    std::vector<int> shifts;
    std::vector<int> g_heights;
    double tot_height = 0;

    for (auto tp : transform_paragraphs) {
        std::vector<std::vector<std::tuple<Word, double>>> stp = split_paragraph(tp, new_width);
        for (auto x : stp) {
            int height = -1;
            int shift = INT_MAX;
            for (auto y : x) {
                std::vector<glyph_result> glyphs = std::get<0>(y).glyphs;
                if (std::get<0>(y).bounding_rect.height > height) {
                    height = std::get<0>(y).bounding_rect.height;
                    auto bounding_rect = std::get<0>(y).bounding_rect;
                }
                for (auto gr : glyphs) {
                    if (gr.shift < shift) {
                        shift = gr.shift;
                    }
                }
            }
            shifts.push_back(shift);
            g_heights.push_back(height);
            tot_height += height;
            if (height > 0) {
                stps.push_back(x);
            }

        }

    }


    double height_coef = 1.5;

    int total_height = margin;
    std::vector<int> line_heights;
    double av_height = tot_height / g_heights.size();

    for (int i=0;i<g_heights.size(); i++) {
        int h = g_heights[i];
        if (h > av_height) {
            total_height += (int)(height_coef * h);
            line_heights.push_back((int)(height_coef * h));
        } else {
            total_height += (int)(height_coef * av_height);
            line_heights.push_back((int)(height_coef * av_height));
        }
    }

    int mat_height = total_height + margin;
    int mat_width  = (int)(new_width + 2*margin);
    cv::Mat new_image(Size(mat_width, mat_height),CV_8UC1, cv::Scalar(255));
    int current_height = margin;

    for (int i=0; i<stps.size(); i++) {
        int h_ = line_heights[i];
        int left = margin;
        std::vector<std::tuple<Word, double>> p = stps[i];
        for (auto e : p) {
            Word w_ = get<0>(e);
            double g_ = get<1>(e);
            std::vector<glyph_result> glyphs = w_.glyphs;
            int bs = INT_MAX;
            for (glyph_result gr : glyphs) {
                if (gr.shift < bs) {
                    bs = gr.shift;
                }
            }
            Mat srcRoi = img(cv::Rect(w_.bounding_rect.x, w_.bounding_rect.y, w_.bounding_rect.width, w_.bounding_rect.height));
            Mat dstRoi = new_image(cv::Rect(left, current_height + (int)(0.2 * h_) - w_.bounding_rect.height - bs, w_.bounding_rect.width, w_.bounding_rect.height));
            srcRoi.copyTo(dstRoi);
            left += (int)g_ + w_.bounding_rect.width;

        }
        current_height += h_;

    }
    //return new_image.clone();
    return new_image;
}
std::vector<words_struct> find_ordered_glyphs(
    std::vector<cv::Rect>& joined_rects) {


    int joined_rects_size = joined_rects.size();

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
    double sum_of_heights = 0.0;

    for (int i = 0; i < size; i++) {
        q[i][0] = joined_rects[i].x + joined_rects[i].width / 2.0;
        q[i][1] = joined_rects[i].y + joined_rects[i].height / 2.0;
        sum_of_heights += joined_rects[i].height;
    }

    double average_height = sum_of_heights / size;

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
        find_neighbors(joined_rects, nearest_neighbors, average_height);

    NeighborsGraph g;

    for (auto it = neighbors.begin(); it != neighbors.end(); it++) {
        auto k = it->first;
        auto v = it->second;
        int nb = std::get<1>(v);
        if (std::get<0>(v).x == -1) {
            add_edge(k, k, g);
        } else {
            add_edge(k, nb, g);
        }
    }


    std::vector<int> c(num_vertices(g));

    connected_components(
        g, make_iterator_property_map(c.begin(), get(vertex_index, g), c[0]));


    std::map<int, std::vector<int>> cc;
    for (int i = 0; i < c.size(); i++) {
        if (cc.find(c[i]) == cc.end()) {
            std::vector<int> v1;
            v1.push_back(i);
            cc[c[i]] = v1;
        } else {
            auto v2 = cc[c[i]];
            v2.push_back(i);
            cc[c[i]] = v2;
        }
    }

    std::set<int> map_keys;

    for (auto it = cc.begin(); it != cc.end(); ++it) {
        int key = it->first;
        map_keys.insert(key);
    }


    std::vector<std::tuple<int, int, int>> line_limits;
   // std::set<int>::iterator it = map_keys.begin();
    // int ind = 0;
    for (int k :  map_keys) {
        //int k = (*it);

        int min1 = INT_MAX;
        int max1 = -1;
        for (int j = 0; j<cc[k].size(); j++) {
            cv::Rect r = joined_rects[cc[k][j]];

            if (r.y < min1) {
                min1 = r.y;
            }
            if (r.y + r.height > max1) {
                max1 = r.y + r.height;
            }
        }
        line_limits.push_back(std::make_tuple(min1, max1, k));
        // ind++;
        //it++;
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

double dist(std::tuple<int, int> p1, std::tuple<int, int> p2) {
    int x1 = std::get<0>(p1);
    int y1 = std::get<1>(p1);
    int x2 = std::get<0>(p2);
    int y2 = std::get<1>(p2);
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
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
        std::vector<std::tuple<int, int>> points1{
            std::make_tuple(r1.x, r1.y),
            std::make_tuple(r1.x, r1.y + r1.height),
            std::make_tuple(r1.x + r1.width, r1.y),
            std::make_tuple(r1.x + r1.width, r1.y + r1.height)};
        std::vector<std::tuple<int, int>> points2{
            std::make_tuple(r2.x, r2.y),
            std::make_tuple(r2.x, r2.y + r2.height),
            std::make_tuple(r2.x + r2.width, r2.y),
            std::make_tuple(r2.x + r2.width, r2.y + r2.height)};

        for (auto p : points1) {
            for (auto q : points2) {
                distances.push_back(dist(p, q));
            }
        }

        auto min_value = *std::min_element(distances.begin(), distances.end());
        dist_info di = {min_value, position};
        return di;
    }
    if (no_x_overlap && !no_y_overlap) {
        int x1 = r1.x;
        int x2 = r1.x + r1.width;
        int x3 = r2.x;
        int x4 = r2.x + r2.width;
        position = r1.x < r2.x ? Position::LEFT : Position::RIGHT;
        std::vector<int> distances{std::abs(x1 - x3), std::abs(x1 - x4),
                                   std::abs(x2 - x3), std::abs(x2 - x4)};
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
        std::vector<int> distances{std::abs(x1 - x3), std::abs(x1 - x4),
                                   std::abs(x2 - x3), std::abs(x2 - x4)};
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
    for (int i = 0; i < sc.size(); i++) {
        int n = sc[i];
        cv::Rect r = jr[n];
        rects.push_back(r);
    }

    std::set<int> inds;
    for (int i = 0; i < rects.size(); i++) {
        inds.insert(i);
    }

    cv::Rect br = bounding_rect(inds, rects);
    cv::Rect pic = jr[pi];
    return dist_rects(br, pic);
}

std::map<int, std::vector<std::vector<int>>> detect_belonging_captions(
    cv::Mat& mat, std::vector<cv::Rect>& jr, std::vector<int>& pic_inds,
    std::vector<std::vector<int>> small_components, double most_frequent_height,
    double multiplier) {
    std::map<int, std::vector<std::vector<int>>> belongs;
    for (int k = 0; k < small_components.size(); k++) {
        std::vector<std::tuple<double, int, Position>> ds;
        std::vector<int> sc = small_components[k];
        for (int j = 0; j < pic_inds.size(); j++) {
            int pi = pic_inds[j];
            dist_info di = distance(sc, pi, jr);
            if (di.pos != Position::UNKNOWN) {
                // add check if there is nothing aroud this component
                // create bounding rect for small components

                std::set<int> rect_inds;
                std::vector<cv::Rect> to_join;
                for (int l = 0; l < sc.size(); l++) {
                    rect_inds.insert(l);
                    to_join.push_back(jr[sc[l]]);
                }

                cv::Rect br = bounding_rect(rect_inds, to_join);

                int left_count = 0;
                int right_count = 0;


                int width = mat.cols;
                int height = mat.rows;
                int extension = 3 * most_frequent_height; //(int)(most_frequent_height / 2);
                int left = br.x - extension > 0
                               ? br.x - extension
                               : 0;
                int right = br.x + br.width + extension < width
                                ? br.x + br.width + extension
                                : width - 1;

                int upper = br.y - extension > 0
                                ? br.y - extension
                                : 0;
                int lower =
                    br.y + br.height + extension < height
                        ? br.y + br.height + extension
                        : height - 1;


                for (int t=0; t<jr.size(); t++) {
                    double a = jr[t].x + jr[t].width / 2.0;
                    double b = jr[t].y + jr[t].height / 2.0;
                    if ((a >= left) && (a <= br.x) && (b >= br.y) && (b <= br.y + br.height)) {
                        left_count++;
                    }

                    if ((a >= br.x + br.width) && (a <= right) && (b >= br.y) && (b <= br.y + br.height)) {
                        right_count++;
                    }

                }

                bool is_caption = left_count == 0 && right_count == 0;//(ratio < 0.008);

                // bool is_caption = (left_sum + upper_sum + right_sum < 10) ||
                // (upper_sum + right_sum + lower_sum < 10) || (right_sum +
                // lower_sum + left_sum < 10) || (lower_sum + left_sum +
                // upper_sum < 10);
                //
                cv::Rect picture_rect = jr[pi];
                int x1 = picture_rect.x;
                int x2 = picture_rect.x + picture_rect.width;

                bool passes_horizontally = (abs(br.x - x1) + abs(br.x + br.width - x2) < picture_rect.width ) || (br.x <= x1 && br.x + br.width >= x2) || (br.x >= x1 && br.x + br.width <= x2);
                
                bool is_line = sc.size() < 5 && ((br.width / (double)(br.height) > 25) || (br.height / (double)(br.width) > 25));

                bool cond = passes_horizontally && !is_line && is_caption &&
                    br.height < multiplier * most_frequent_height;

                if (passes_horizontally && !is_line && is_caption &&
                    br.height < multiplier * most_frequent_height) {
                    ds.push_back(std::make_tuple(di.distance, pi, di.pos));
                }
            }
        }
        if (ds.size() > 0) {
            std::sort(ds.begin(), ds.end());
            std::tuple<double, int, Position> t = ds[0];
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

std::vector<int> join_with_captions(std::map<int, std::vector<std::vector<int>>>& belongs,
                        std::vector<cv::Rect>& rects,
                        std::vector<cv::Rect>& output) {
    std::vector<cv::Rect> ret_val;

    std::vector<int> belong_keys;
    for (auto const& itmap : belongs) {
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
        for (int l = 0; l < to_join.size(); l++) {
            rect_inds.insert(l);
        }

        cv::Rect br = bounding_rect(rect_inds, to_join);
        ret_val.push_back(br);
    }

    for (int i = 0; i < rects.size(); i++) {
        if (std::find(inds_to_ignore.begin(), inds_to_ignore.end(), i) ==
            inds_to_ignore.end()) {
            ret_val.push_back(rects[i]);
        }
    }

    std::vector<cv::Rect> new_rects = join_rects(ret_val);
    double s = 0.0;
    for (int i = 0; i<new_rects.size(); i++) {
        s += new_rects[i].height;
        output.push_back(new_rects[i]);
    }

    double average_height = s / new_rects.size();

    std::vector<int> pic_indexes;

    for (int i = 0; i<new_rects.size(); i++) {
        if (new_rects[i].height > 7 * average_height) {
            pic_indexes.push_back(i);
        }
    }
    
    return pic_indexes;

}

std::map<int, std::vector<std::vector<int>>> detect_captions(
    cv::Mat& mat, std::vector<cv::Rect>& joined_rects) {
    std::map<int, std::vector<std::vector<int>>> belongs;
    double s = 0.0;
    int n = joined_rects.size();
    std::vector<int> pic_inds;
    std::vector<int> heights;
    for (int i = 0; i < n; i++) {
        cv::Rect jr = joined_rects[i];
        s += jr.height;
        heights.push_back(jr.height);
    }


    double most_frequent_height = s / joined_rects.size();
    for (int i = 0; i < n; i++) {
        cv::Rect jr = joined_rects[i];
        if (jr.height > 7 * most_frequent_height) {
            pic_inds.push_back(i);
        }
    }
    double* distmat = new double[(n * (n - 1)) / 2];
    int i, k, j;
    for (i = k = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            // compute distance between observables i and j
            double x1 = (joined_rects[i].x + joined_rects[i].width) / 2;
            double y1 = (joined_rects[i].y + joined_rects[i].height) / 2;
            double x2 = (joined_rects[j].x + joined_rects[j].width) / 2;
            double y2 = (joined_rects[j].y + joined_rects[j].height) / 2;
            distmat[k] = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
            k++;
        }
    }
    int* merge = new int[2 * (n - 1)];
    double* height = new double[n - 1];
    hclust_fast(n, distmat, HCLUST_METHOD_SINGLE, merge, height);

    int* labels = new int[n];

    int last_cluster_count = std::numeric_limits<int>::max();
    double x = 0.5;
    while (x <= 4) {
        double coef = x;
        x += 0.1;
        cutree_cdist(n, merge, height, most_frequent_height * coef, labels);

        std::map<int, std::vector<int>> res;
        std::set<int> keys;

        for (int t = 0; t < n; t++) {
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
                for (auto pi : pic_inds) {
                    sc.erase(std::remove(sc.begin(), sc.end(), pi),
                             sc.end());
                }
                if (sc.size() > 0 && sc.size() < 100) {
                    small_components.push_back(sc);
                }
            }


            belongs = detect_belonging_captions(mat, joined_rects, pic_inds,
                                                small_components,
                                                most_frequent_height, coef * 6);
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
