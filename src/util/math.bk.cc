#include <cstring>
#include <limits>
#include <algorithm>

#include "math.h"

using std::min;
using std::max;

namespace pdf2htmlEX {

void tm_transform(const double * tm, double & x, double & y, bool is_delta)
{
    double xx = x, yy = y;
    x = tm[0] * xx + tm[2] * yy;
    y = tm[1] * xx + tm[3] * yy;
    if(!is_delta)
    {
        x += tm[4];
        y += tm[5];
    }
}

void tm_multiply(const double * tm_left, const double * tm_right, double * tm_result)
{
    double old[4];
    memcpy(old, tm_left, sizeof(old));

    tm_result[0] = tm_left[0] * tm_right[0] + tm_left[2] * tm_right[1];
    tm_result[1] = tm_left[1] * tm_right[0] + tm_left[3] * tm_right[1];
    tm_result[2] = tm_left[0] * tm_right[2] + tm_left[2] * tm_right[3];
    tm_result[3] = tm_left[1] * tm_right[2] + tm_left[3] * tm_right[3];
    tm_result[4] += tm_left[0] * tm_right[4] + tm_left[2] * tm_right[5];
    tm_result[5] += tm_left[1] * tm_right[4] + tm_left[3] * tm_right[5];
}

void tm_transform_bbox(const double * tm, double * bbox)
{
    double & x1 = bbox[0];
    double & y1 = bbox[1];
    double & x2 = bbox[2];
    double & y2 = bbox[3];
    double _[4][2];
    _[0][0] = _[1][0] = x1;
    _[0][1] = _[2][1] = y1;
    _[2][0] = _[3][0] = x2;
    _[1][1] = _[3][1] = y2;

    x1 = y1 = std::numeric_limits<double>::max();
    x2 = y2 = std::numeric_limits<double>::min();
    for(int i = 0; i < 4; ++i)
    {
        auto & x = _[i][0];
        auto & y = _[i][1];
        tm_transform(tm, x, y);
        if(x < x1) x1 = x;
        if(x > x2) x2 = x;
        if(y < y1) y1 = y;
        if(y > y2) y2 = y;
    }
}

bool bbox_intersect(double * bbox1, double * bbox2)
{
    return min(bbox1[0], bbox1[2]) < max(bbox2[0], bbox2[2])
        && max(bbox1[0], bbox1[2]) > min(bbox2[0], bbox2[2])
        && min(bbox1[1], bbox1[3]) < max(bbox2[1], bbox2[3])
        && max(bbox1[1], bbox1[3]) > min(bbox2[1], bbox2[3]);
}

} //namespace pdf2htmlEX 

