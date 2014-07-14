/*
 * Math functions
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef MATH_H__
#define MATH_H__

#include <cmath>

#include "const.h"

namespace pdf2htmlEX {

static inline double round(double x) { return (std::abs(x) > EPS) ? x : 0.0; }
static inline bool equal(double x, double y) { return std::abs(x-y) <= EPS; }
static inline bool is_positive(double x) { return x > EPS; }
static inline bool tm_equal(const double * tm1, const double * tm2, int size = 6)
{
    for(int i = 0; i < size; ++i)
        if(!equal(tm1[i], tm2[i]))
            return false;
    return true;
}

static inline void tm_init(double * tm)
{
    tm[0] = tm[3] = 1;
    tm[1] = tm[2] = tm[4] = tm[5] = 0;
}

static inline void tm_multiply(double * result, const double * m1, const double * m2)
{
    result[0] = m1[0] * m2[0] + m1[2] * m2[1];
    result[1] = m1[1] * m2[0] + m1[3] * m2[1];
    result[2] = m1[0] * m2[2] + m1[2] * m2[3];
    result[3] = m1[1] * m2[2] + m1[3] * m2[3];
    result[4] = m1[0] * m2[4] + m1[2] * m2[5] + m1[4];
    result[5] = m1[1] * m2[4] + m1[3] * m2[5] + m1[5];
}

static inline double hypot(double x, double y) { return std::sqrt(x*x+y*y); }

void tm_transform(const double * tm, double & x, double & y, bool is_delta = false);
void tm_multiply(double * tm_left, const double * tm_right);
void tm_transform_bbox(const double * tm, double * bbox);
/**
 * Calculate the intersection of 2 boxes.
 * If they are intersecting, store the result to result (if not null) and return true.
 * Otherwise return false, and result is not touched.
 * Param result can be same as one of bbox1 and bbox2.
 * Data in boxes are expected in the order of (x0, y0, x1, y1).
 */
bool bbox_intersect(const double * bbox1, const double * bbox2, double * result = nullptr);

} //namespace pdf2htmlEX 
#endif //MATH_H__
