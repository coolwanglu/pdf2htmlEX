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
    bool operator == (const Color & c) const {
        if(transparent != c.transparent)
            return false;
        if(transparent)
            return true;
        return ((rgb.r == c.rgb.r) && (rgb.g == c.rgb.g) && (rgb.b == c.rgb.b));
    }
};

std::ostream & operator << (std::ostream & out, const Color & color);

} // namespace pdf2htmlEX

#endif // COLOR_H__
