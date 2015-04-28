// StyleNode.h: represents styles for text
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>
#ifndef TEXTSTYLE_H__
#define TEXTSTYLE_H__

#include <cairo.h>


namespace pdf2htmlEX {
 
struct TextStyle
{
    // combination of CTM, TextMatrix, font_size, HorizontalScaling, rise
    cairo_matrix_t matrix;

    // font 

    // matrix is that (combined) matrix in PDF scaled by 1/font_size
    // which is usually used for canonicalization/optimization
    double font_size;

    Color fill_color;
    Color stroke_color;
    double letter_space;
    double word_space;
};

} // namespace pdf2htmlEX 

#endif // TEXTSTYLE_H__
