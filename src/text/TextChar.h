// TextChar: holds a char
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>

#ifndef TEXTCHAR_H__
#define TEXTCHAR_H__

#include <vector>

#include "CharTypes.h"

namespace pdf2htmlEX {

struct TextChar
{
    TextStyle const* style = nullptr;
    
    // the bounding box for the glyph, in text coordinates
    // x0,y0,x1,y1
    double bbox[4];

    // the location and shifts of the origin/base, in text coordinates
    // font_size is applied
    double x, y, dx, dy; 

    // the width of current char, as well as possibly letter_space and word_space
    // font_size is applied
    double width;

    int order_number;

    // original char code
    CharCode code;
    std::vector<Unicode> unicodes;
};


} // namespace pdf2htmlEX

#endif // TEXTCHAR_H__
