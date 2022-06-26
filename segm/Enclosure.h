
#ifndef FLOW_READER_ENCLOSURE_H
#define FLOW_READER_ENCLOSURE_H
#include "common.h"



struct ArrayOrder
{
    explicit ArrayOrder(int i) : i(i) {}

    virtual bool operator()(std::array<int, 4> const &lhs, std::array<int, 4> const &rhs) const = 0;

protected:
    int i;
};

struct TupleOrder
{
    bool operator()(std::tuple<std::array<int, 4>, char> const &lhs, std::tuple<std::array<int, 4>, char> const &rhs) const
    {
        return std::get<0>(lhs)[2] > std::get<0>(rhs)[2];
    }
};

struct CustomLessThan : public ArrayOrder
{
    CustomLessThan(int i) : ArrayOrder(i) {}

    bool operator()(std::array<int, 4> const &lhs, std::array<int, 4> const &rhs) const
    {
        return lhs[i] < rhs[i];
    }
};


class Enclosure
{
public:
    Enclosure(std::vector<std::array<int, 4>>& points);
    Enclosure(std::vector<Rect>& rects);

    std::set<std::array<int, 4>> solve();

    virtual ~Enclosure();

private:
    std::vector<std::array<int, 4>> points;
    std::set<std::array<int, 4>> all_;
    std::map<int,std::map<int, int>> elementsMap;
    std::map<int, std::vector<int>> mapNumberToPos;

    void normalize();

    std::vector<std::tuple<std::array<int, 4>, std::array<int, 4>>> report(std::vector<std::array<int, 4>>);

    std::vector<std::tuple<std::array<int, 4>, std::array<int, 4>>> merge(std::vector<std::array<int, 4>>, std::vector<std::array<int, 4>>);

    inline float getMedian(std::vector<std::array<int, 4>> points)
    {
        sort(points.begin(), points.end(), CustomLessThan(3));
        float med = 0.0;
        size_t size = points.size();
        unsigned long ind = size / 2;
        if (size % 2 == 0)
        {
            med = (points[ind - 1][3] + points[ind][3]) / 2.0;
        }
        else
        {
            med = points[ind][3];
        }
        return med;
    }

    int d = 4;
    float med = 0.0;
};




#endif //FLOW_READER_ENCLOSURE_H
