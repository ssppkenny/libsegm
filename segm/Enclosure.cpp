
#include "Enclosure.h"

Enclosure::~Enclosure() {

}

vector<std::tuple<array<int, 4>, array<int, 4>>>
Enclosure::merge(vector<array<int, 4>> s1, vector<array<int, 4>> s2) {
    sort(s1.begin(), s1.end(), CustomLessThan(2));
    sort(s2.begin(), s2.end(), CustomLessThan(2));

    vector<std::tuple<array<int, 4>, char>> new_s1;
    vector<std::tuple<array<int, 4>, char>> new_s2;

    for (int i = 0; i < s1.size(); i++) {
        new_s1.push_back(std::tuple<array<int, 4>, char>(s1[i], 'r'));
    }

    for (int i = 0; i < s2.size(); i++) {
        new_s2.push_back(std::tuple<array<int, 4>, char>(s2[i], 'b'));
    }

    vector<std::tuple<array<int, 4>, char>> all_s;

    vector<array<int, 4>> heap;

    vector<std::tuple<array<int, 4>, array<int, 4>>> ret;

    all_s.reserve(new_s1.size() + new_s2.size());
    all_s.insert(all_s.end(), new_s1.begin(), new_s1.end());
    all_s.insert(all_s.end(), new_s2.begin(), new_s2.end());
    sort(all_s.begin(), all_s.end(), TupleOrder());

    for (int i = 0; i < all_s.size(); i++) {
        if (get<1>(all_s[i]) == 'b') {
            heap.push_back(get<0>(all_s[i]));
        } else {
            array<int, 4> se = get<0>(all_s[i]);
            for (int j = 0; j < heap.size(); j++) {
                array<int, 4> e = heap[j];
                if (e[0] >= se[0] && e[1] >= se[1]) {
                    ret.push_back(std::tuple<array<int, 4>, array<int, 4>>(se, e));
                }
            }
        }
    }

    return ret;
};

vector<std::tuple<array<int, 4>, array<int, 4>>> Enclosure::report(vector<array<int, 4>> points) {
    size_t size = points.size();
    if (size <= 1) {
        vector<std::tuple<array<int, 4>, array<int, 4>>> v;
        return v;
    } else if (size == 2) {
        vector<std::tuple<array<int, 4>, array<int, 4>>> v;
        array<int, 4> a = points[0];
        array<int, 4> b = points[1];
        if (get<0>(a) < get<0>(b) && get<1>(a) < get<1>(b) && get<2>(a) < get<2>(b) &&
                get<3>(a) < get<3>(b)) {
            v.push_back(std::tuple<array<int, 4>, array<int, 4>>(a, b));
            return v;
        } else if (get<0>(a) >= get<0>(b) && get<1>(a) >= get<1>(b) && get<2>(a) >= get<2>(b) &&
                get<3>(a) >= get<3>(b)) {
            v.push_back(std::tuple<array<int, 4>, array<int, 4>>(b, a));
            return v;
        } else {
            return v;
        }
    } else if (size == 3) {
        vector<std::tuple<array<int, 4>, array<int, 4>>> v;
        array<int, 4> a = points[0];
        array<int, 4> b = points[1];
        array<int, 4> c = points[2];

        if (get<0>(a) < get<0>(b) && get<1>(a) < get<1>(b) && get<2>(a) < get<2>(b) &&
                get<3>(a) < get<3>(b)) {
            v.push_back(std::tuple<array<int, 4>, array<int, 4>>(a, b));
        } else if (get<0>(a) >= get<0>(b) && get<1>(a) >= get<1>(b) && get<2>(a) >= get<2>(b) &&
                get<3>(a) >= get<3>(b)) {
            v.push_back(std::tuple<array<int, 4>, array<int, 4>>(b, a));
        }

        if (get<0>(a) < get<0>(c) && get<1>(a) < get<1>(c) && get<2>(a) < get<2>(c) &&
                get<3>(a) < get<3>(c)) {
            v.push_back(std::tuple<array<int, 4>, array<int, 4>>(a, c));
        } else if (get<0>(a) >= get<0>(c) && get<1>(a) >= get<1>(c) && get<2>(a) >= get<2>(c) &&
                get<3>(a) >= get<3>(c)) {
            v.push_back(std::tuple<array<int, 4>, array<int, 4>>(c, a));
        }

        if (get<0>(b) < get<0>(c) && get<1>(b) < get<1>(c) && get<2>(b) < get<2>(c) &&
                get<3>(b) < get<3>(c)) {
            v.push_back(std::tuple<array<int, 4>, array<int, 4>>(b, c));
        } else if (get<0>(b) >= get<0>(c) && get<1>(b) >= get<1>(c) && get<2>(b) >= get<2>(c) &&
                get<3>(b) >= get<3>(c)) {
            v.push_back(std::tuple<array<int, 4>, array<int, 4>>(c, b));
        }

        return v;


    } else {
        float m = getMedian(points);
        vector<array<int, 4>> s1;
        vector<array<int, 4>> s2;
        copy_if(points.begin(), points.end(), std::back_inserter(s1),
                [m](array<int, 4> a) { return a[3] <= m; });
        copy_if(points.begin(), points.end(), std::back_inserter(s2),
                [m](array<int, 4> a) { return a[3] > m; });
        vector<std::tuple<array<int, 4>, array<int, 4>>> x = report(s1);
        vector<std::tuple<array<int, 4>, array<int, 4>>> y = report(s2);
        vector<std::tuple<array<int, 4>, array<int, 4>>> merged = merge(s1, s2);

        vector<std::tuple<array<int, 4>, array<int, 4>>> v;
        size_t s = x.size() + y.size() + merged.size();
        v.reserve(s);
        v.insert(v.end(), x.begin(), x.end());
        v.insert(v.end(), y.begin(), y.end());
        v.insert(v.end(), merged.begin(), merged.end());
        return v;

    }
};


set<array<int, 4>> Enclosure::solve() {
    vector<std::tuple<array<int, 4>, array<int, 4>>> out = report(points);

    set<array<int, 4>> big;
    set<array<int, 4>> small;
    for (int i = 0; i < out.size(); i++) {

        array<int, 4> b = get<1>(out[i]);
        array<int, 4> s = get<0>(out[i]);
        for (int j = 0; j < 4; j++) {
            b[j] = elementsMap[j][b[j]];
            s[j] = elementsMap[j][s[j]];
        }

        big.insert(b);
        small.insert(s);
    }

    set<array<int, 4>> diff;
    set<array<int, 4>> diff1;
    set<array<int, 4>> diff2;
    set<array<int, 4>> union_;
    set<array<int, 4>> union__;

    set_union(big.begin(), big.end(), small.begin(), small.end(),
            std::inserter(union_, union_.begin()));

    set_difference(all_.begin(), all_.end(), union_.begin(), union_.end(),
            std::inserter(diff1, diff1.begin()));


    set_difference(big.begin(), big.end(), small.begin(), small.end(),
            std::inserter(diff, diff.begin()));

    set_union(diff1.begin(), diff1.end(), diff.begin(), diff.end(),
            std::inserter(union__, union__.begin()));

    set_difference(union__.begin(), union__.end(), small.begin(), small.end(),
            std::inserter(diff2, diff2.begin()));

    return diff2;
}

Enclosure::Enclosure(vector<array<int, 4>> &points) {

    this->points = points;
    std::copy(points.begin(), points.end(), std::inserter(all_, all_.end()));

    normalize();
}

Enclosure::Enclosure(vector<Rect> &rects) {

    points = vector<array<int, 4>>();

    for (int i = 0; i < rects.size(); i++) {
        Rect rect = rects.at(i);
        array<int, 4> a{{-rect.x, -rect.y, rect.x + rect.width, rect.y + rect.height}};
        points.push_back(a);
    }

    std::copy(points.begin(), points.end(), std::inserter(all_, all_.end()));

    normalize();
}

void Enclosure::normalize() {
    size_t size = this->points.size();

    vector<CustomLessThan> orders;
    CustomLessThan order0(0);
    CustomLessThan order1(1);
    CustomLessThan order2(2);
    CustomLessThan order3(3);

    orders.push_back(order0);
    orders.push_back(order1);
    orders.push_back(order2);
    orders.push_back(order3);

    for (int i = 0; i < size; i++) {
        mapNumberToPos[0].push_back(get<0>(this->points[i]));
        mapNumberToPos[1].push_back(get<1>(this->points[i]));
        mapNumberToPos[2].push_back(get<2>(this->points[i]));
        mapNumberToPos[3].push_back(get<3>(this->points[i]));
    }

    for (int j = 0; j < 4; j++) {
        sort(mapNumberToPos[j].begin(), mapNumberToPos[j].end());
        map<int, int> lm;
        elementsMap[j] = lm;
    }


    for (int j = 0; j < 4; j++) {
        size_t size = mapNumberToPos[j].size();
        sort(this->points.begin(), this->points.end(), orders[j]);
        for (int k = 0; k < size; k++) {
            elementsMap[j][k] = mapNumberToPos[j][k];
            this->points[k][j] = k;
        }
    }

    med = getMedian(points);

}



