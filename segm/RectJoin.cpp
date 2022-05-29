#include "RectJoin.h"

struct less_than_event {
    inline bool operator()(const sweep_event& e1, const sweep_event& e2) {
        if (e1.x == e2.x) {
            return e1.type < e2.type;
        }

        return e1.x < e2.x;
    }
};

struct less_than_rect {
    inline bool operator()(const cv::Rect& r1, const cv::Rect& r2) {
        return r1.x < r2.x;
    }
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, int>
    RectsGraph;
typedef boost::graph_traits<RectsGraph>::vertex_descriptor vertex_tt;

std::vector<std::pair<interval<int>, interval<int>>> all_pairs(
    std::vector<interval<int>> intervals) {
    int size = intervals.size();
    std::vector<std::pair<interval<int>, interval<int>>> return_value;
    for (int i = 0; i < size; i++) {
        for (int j = i + 1; j < size; j++) {
            return_value.push_back(
                std::make_pair(intervals.at(i), intervals.at(j)));
        }
    }
    return return_value;
}

std::vector<cv::Rect> join_rects(vector<cv::Rect>& rects) {
    std::vector<cv::Rect> joined_rects;
    std::sort(rects.begin(), rects.end(), less_than_rect());

    std::map<std::array<int, 4>, int> numbered_rects;
    std::map<std::pair<int, int>, std::vector<cv::Rect>> rects_dict;
    std::vector<sweep_event> events;
    RectsGraph g;
    interval_tree_t<int> tree;
    int i = 0;
    for (const auto& value : rects) {
        std::array<int, 4> a = {value.x, value.y, value.width, value.height};
        numbered_rects[a] = i;
        add_vertex(i, g);
        i++;
        std::pair<int, int> map_key =
            std::make_pair(value.y, value.y + value.height);
        if (rects_dict.find(map_key) == rects_dict.end()) {
            std::vector<cv::Rect> v;
            v.push_back(value);
            rects_dict[map_key] = v;
        } else {
            auto& v = rects_dict[map_key];
            v.push_back(value);
            rects_dict[map_key] = v;
        }
        sweep_event le;
        le.x = value.x;
        le.rect = value;
        le.type = Left;
        events.push_back(le);
        sweep_event re;
        re.x = value.x + value.width;
        re.rect = value;
        re.type = Right;
        events.push_back(re);
    }

    std::sort(events.begin(), events.end(), less_than_event());

    for (const auto& event : events) {
        cv::Rect rect = event.rect;
        if (event.type == Left) {
            std::vector<interval<int>> overlapping_intervals;
            tree.overlap_find_all(
                {rect.y, rect.y + rect.height},
                [&overlapping_intervals](auto iter) {
                    overlapping_intervals.push_back(
                        make_safe_interval(iter->low(), iter->high()));
                    return true;
                });
            if (overlapping_intervals.size() > 0) {
                overlapping_intervals.push_back(
                    make_safe_interval(rect.y, rect.y + rect.height));

                std::vector<std::pair<interval<int>, interval<int>>> pairs =
                    all_pairs(overlapping_intervals);
                for (const auto& p : pairs) {
                    std::vector<cv::Rect> r1filtered;
                    std::vector<cv::Rect> r2filtered;
                    std::vector<cv::Rect> r1s = rects_dict[std::make_pair(
                        p.first.low(), p.first.high())];
                    std::copy_if(
                        r1s.begin(), r1s.end(), std::back_inserter(r1filtered),
                        [&event](cv::Rect r) {
                            return event.x <= r.x + r.width && event.x >= r.x;
                        });
                    cv::Rect r1 = r1filtered.at(0);
                    std::vector<cv::Rect> r2s = rects_dict[std::make_pair(
                        p.second.low(), p.second.high())];
                    std::copy_if(
                        r2s.begin(), r2s.end(), std::back_inserter(r2filtered),
                        [&event](cv::Rect r) {
                            return event.x <= r.x + r.width && event.x >= r.x;
                        });
                    cv::Rect r2 = r2filtered.at(0);
                    int ind1 = numbered_rects.at(
                        std::array<int, 4>{r1.x, r1.y, r1.width, r1.height});
                    int ind2 = numbered_rects.at(
                        std::array<int, 4>{r2.x, r2.y, r2.width, r2.height});
                    add_edge(ind1, ind2, g);
                }
            }
            tree.insert(make_safe_interval(rect.y, rect.y + rect.height));
        } else {
            auto interval = make_safe_interval(rect.y, rect.y + rect.height);
            auto it = tree.find(interval);
            tree.erase(it);
        }
    }

    std::vector<int> c(num_vertices(g));
    connected_components(
        g, make_iterator_property_map(c.begin(), get(vertex_index, g), c[0]));
    std::map<int, std::vector<int>> components;

    for (int i = 0; i < c.size(); i++) {
        int cn = c[i];
        if (components.find(cn) != components.end()) {
            std::vector<int> v = components[cn];
            v.push_back(i);
            components[cn] = v;
        } else {
            std::vector<int> v;
            v.push_back(i);
            components[cn] = v;
        }
    }

    std::set<int> map_keys;

    for (auto it = components.begin(); it != components.end(); ++it) {
        map_keys.insert(it->first);
    }

    std::set<int>::iterator it = map_keys.begin();
    while (it != map_keys.end()) {
        // Print the element
        int k = (*it);
        std::vector<int> cc = components.at(k);
        int minx = INT_MAX, miny = INT_MAX;
        int maxx = 0, maxy = 0;
        for (auto i : cc) {
            cv::Rect r = rects.at(i);
            if (r.x < minx) {
                minx = r.x;
            }
            if (r.x + r.width > maxx) {
                maxx = r.x + r.width;
            }
            if (r.y < miny) {
                miny = r.y;
            }
            if (r.y + r.height > maxy) {
                maxy = r.y + r.height;
            }
        }
        joined_rects.push_back(cv::Rect(minx, miny, maxx - minx, maxy - miny));

        it++;
    }

    return joined_rects;
}

