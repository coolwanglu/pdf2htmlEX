/*
 * Header file for Color 
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef COLOR_H__
#define COLOR_H__

#include <ostream>

#include <GfxState.h>

namespace pdf2htmlEX {

struct Color
{
    bool transparent;
    GfxRGB rgb;
    Color();
    Color(double r, double g, double b, bool transparent = false);
    Color(const GfxRGB& rgb);
    bool operator == (const Color & c) const {
        if(transparent != c.transparent)
            return false;
        if(transparent)
            return true;
        return ((rgb.r == c.rgb.r) && (rgb.g == c.rgb.g) && (rgb.b == c.rgb.b));
    }
    void get_gfx_color(GfxColor & gc) const;
    // Color distance, [0,1].
    double distance(const Color & other) const;
};

std::ostream & operator << (std::ostream & out, const Color & color);

} // namespace pdf2htmlEX

#endif // COLOR_H__
