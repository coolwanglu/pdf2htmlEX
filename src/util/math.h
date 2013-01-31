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
static inline double hypot(double x, double y) { return std::sqrt(x*x+y*y); }

void tm_transform(const double * tm, double & x, double & y, bool is_delta = false);
void tm_multiply(double * tm_left, const double * tm_right);

} //namespace pdf2htmlEX 
#endif //MATH_H__
