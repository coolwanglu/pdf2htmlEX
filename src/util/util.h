/*
 * Help classes and Functions
 *
 * by WangLu
 * 2012.08.10
 */


#ifndef UTIL_H__
#define UTIL_H__

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <map>

#include <GfxState.h>

#include "const.h"

#ifndef nullptr
#define nullptr (NULL)
#endif

namespace pdf2htmlEX {

static inline double _round(double x) { return (std::abs(x) > EPS) ? x : 0.0; }
static inline bool _equal(double x, double y) { return std::abs(x-y) < EPS; }
static inline bool _is_positive(double x) { return x > EPS; }
static inline bool _tm_equal(const double * tm1, const double * tm2, int size = 6)
{
    for(int i = 0; i < size; ++i)
        if(!_equal(tm1[i], tm2[i]))
            return false;
    return true;
}
static inline double _hypot(double x, double y) { return std::sqrt(x*x+y*y); }

void _tm_transform(const double * tm, double & x, double & y, bool is_delta = false);
void _tm_multiply(double * tm_left, const double * tm_right);

static inline long long hash_ref(const Ref * id)
{
    return (((long long)(id->num)) << (sizeof(id->gen)*8)) | (id->gen);
}

class GfxRGB_hash 
{
public:
    size_t operator () (const GfxRGB & rgb) const
    {
        return (colToByte(rgb.r) << 16) | (colToByte(rgb.g) << 8) | (colToByte(rgb.b));
    }
};

class GfxRGB_equal
{ 
public:
    bool operator ()(const GfxRGB & rgb1, const GfxRGB & rgb2) const
    {
        return ((rgb1.r == rgb2.r) && (rgb1.g == rgb2.g) && (rgb1.b == rgb1.b));
    }
};

// we may need more info of a font in the future
class FontInfo
{
public:
    long long id;
    bool use_tounicode;
    int em_size;
    double ascent, descent;
};

class Matrix_less
{
public:
    bool operator () (const Matrix & m1, const Matrix & m2) const
    {
        // Note that we only care about the first 4 elements
        for(int i = 0; i < 4; ++i)
        {
            if(m1.m[i] < m2.m[i] - EPS)
                return true;
            if(m1.m[i] > m2.m[i] + EPS)
                return false;
        }
        return false;
    }
};

void create_directories(std::string path);

bool is_truetype_suffix(const std::string & suffix);

std::string get_filename(const std::string & path);
std::string get_suffix(const std::string & path);

/*
 * In PDF, edges of the rectangle are in the middle of the borders
 * In HTML, edges are completely outside the rectangle
 */
void css_fix_rectangle_border_width(double x1, double y1, double x2, double y2, 
        double border_width, 
        double & x, double & y, double & w, double & h,
        double & border_top_bottom_width, 
        double & border_left_right_width);

std::ostream & operator << (std::ostream & out, const GfxRGB & rgb);

} // namespace pdf2htmlEX

#endif //UTIL_H__
