// CharNode: holds a char
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>

#ifndef CHARNODE_H__
#define CHARNODE_H__

#include <cairo.h>

namespace pdf2htmlEX {

struct CharNode
{
    // the bounding box for the glyph, in text coordinates
    cairo_rectangle_t bbox; 

    // the location and shifts of the origin/base, in text coordinates
    double x, y, dx, dy; 

    int order_number;

    // unicode's?
    // original char code?
    // font pointer?
};

} // namespace pdf2htmlEX

#endif // CHARNODE_H__
