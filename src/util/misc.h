/*
 * Help classes and Functions
 *
 * by WangLu
 * 2012.08.10
 */


#ifndef UTIL_H__
#define UTIL_H__

#include <iostream>

#include <GfxState.h>

#include "util/const.h"

namespace pdf2htmlEX {

static inline long long hash_ref(const Ref * id)
{
    return (((long long)(id->num)) << (sizeof(id->gen)*8)) | (id->gen);
}

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

class GfxRGB_hash 
{
public:
    size_t operator () (const GfxRGB & rgb) const
    {
        return ( (((size_t)colToByte(rgb.r)) << 16) 
               | (((size_t)colToByte(rgb.g)) << 8) 
               | ((size_t)colToByte(rgb.b))
               );
    }
};

class GfxRGB_equal
{ 
public:
    bool operator ()(const GfxRGB & rgb1, const GfxRGB & rgb2) const
    {
        return ((rgb1.r == rgb2.r) && (rgb1.g == rgb2.g) && (rgb1.b == rgb2.b));
    }
};


} // namespace pdf2htmlEX

#endif //UTIL_H__
