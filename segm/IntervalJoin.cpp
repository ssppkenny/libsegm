#include "IntervalJoin.h"

struct less_than_interval_event {
    inline bool operator()(const std::tuple<int,int,std::tuple<int,int,int>, int>& e1, const std::tuple<int,int,std::tuple<int,int,int>, int>& e2) {
        if (get<0>(e1) == get<0>(e2)) {
            return get<1>(e1) < get<1>(e2);
        }

        return get<0>(e1) < get<0>(e2);
    }
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, int>
    IntervalsGraph;

std::vector<interval<int>> join_intervals(std::vector<std::tuple<int,int,int>> rngs) {
    std::vector<std::tuple<int,int,std::tuple<int,int,int>,int>> events;

    for (auto x : rngs) {
        events.push_back(std::make_tuple(std::get<0>(x), 0, x, std::get<2>(x)));
        events.push_back(std::make_tuple(std::get<1>(x), 1, x, std::get<2>(x)));
    }


    std::sort(events.begin(), events.end(), less_than_interval_event());

    std::map<std::tuple<int,int,int>, int> queue;
    std::set<std::tuple<int,int,int>> intersecting;

    IntervalsGraph g;

    for (int i=0; i<rngs.size(); i++) {
        boost::add_vertex(i, g);
    }

    for (auto& rng : events) {
        int x = std::get<0>(rng);
        int t = std::get<1>(rng);
        std::tuple<int,int,int> r = std::get<2>(rng);
        int i = std::get<3>(rng);

        if (t == 0) {
            queue[r] = i;
            intersecting.insert(r);
        } else {
            queue.erase(r);
            std::vector<std::tuple<int,int,int>> v(intersecting.size());
            std::copy(intersecting.begin(), intersecting.end(), v.begin());
            std::vector<std::pair<std::tuple<int,int,int>, std::tuple<int, int, int>>> pairs = all_pairs(v);

            for (auto& p : pairs) {
                auto a = p.first;
                auto b = p.second;
                int left = std::max(std::get<0>(a), std::get<0>(b));
                int right = std::min(std::get<1>(a), std::get<1>(b));
                if ((double)(right - left) / (std::get<1>(a) - std::get<0>(a)) > 0.1  && ((double)(right - left) / (std::get<1>(b) - std::get<0>(b)) > 0.1 )) {
                    add_edge(std::get<2>(a), std::get<2>(b), g);
                }
            }
            if (queue.size() == 0) {
                intersecting = std::set<std::tuple<int,int,int>>();
            }
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

    std::vector<interval<int>> ret_val;
    std::set<int>::iterator it = map_keys.begin();
    while (it != map_keys.end()) {
        int k = (*it);
        std::vector<int> cc = components.at(k);
        int min1 = INT_MAX;
        int max1 = 0;
        for (auto i : cc) {
            std::tuple<int,int,int> r = rngs.at(i);

            int mn = std::get<0>(r);
            int mx = std::get<1>(r);

            if (mn < min1) {
                min1 = mn;
            }
            if (mx > max1) {
                max1 = mx;
            }

        }

        ret_val.push_back(make_safe_interval(min1, max1));

        it++;
    }
    

    return ret_val;

}
