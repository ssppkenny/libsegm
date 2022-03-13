
#ifndef FLOW_READER_ENCLOSURE_H
#define FLOW_READER_ENCLOSURE_H
#include "common.h"



struct ArrayOrder {
    explicit ArrayOrder(int i) : i(i) {}

    virtual bool operator()(array<int, 4> const &lhs, array<int, 4> const &rhs) const = 0;

    protected:
    int i;
};

struct TupleOrder {
    bool operator()(std::tuple<array<int, 4>, char> const &lhs, std::tuple<array<int, 4>, char> const &rhs) const {
        return get<0>(lhs)[2] > get<0>(rhs)[2];
    }
};

struct CustomLessThan : public ArrayOrder {
    CustomLessThan(int i) : ArrayOrder(i) {}

    bool operator()(array<int, 4> const &lhs, array<int, 4> const &rhs) const {
        return lhs[i] < rhs[i];
    }
};


class Enclosure {
    public:
        Enclosure(vector<array<int, 4>>& points);
        Enclosure(vector<Rect>& rects);

        set<array<int, 4>> solve();

        virtual ~Enclosure();

    private:
        vector<array<int, 4>> points;
        set<array<int, 4>> all_;
        map<int,map<int, int>> elementsMap;
        map<int, vector<int>> mapNumberToPos;

        void normalize();

        vector<std::tuple<array<int, 4>, array<int, 4>>> report(vector<array<int, 4>>);

        vector<std::tuple<array<int, 4>, array<int, 4>>> merge(vector<array<int, 4>>, vector<array<int, 4>>);

        inline float getMedian(vector<array<int, 4>> points) {
            sort(points.begin(), points.end(), CustomLessThan(3));
            float med = 0.0;
            size_t size = points.size();
            unsigned long ind = size / 2;
            if (size % 2 == 0) {
                med = (points[ind - 1][3] + points[ind][3]) / 2.0;
            } else {
                med = points[ind][3];
            }
            return med;
        }

        int d = 4;
        float med = 0.0;
};




#endif //FLOW_READER_ENCLOSURE_H
