/*
 * Header file for HTMLState
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */
#ifndef HTMLSTATE_H__
#define HTMLSTATE_H__

#include <functional>

#include "Color.h"

namespace pdf2htmlEX {

struct FontInfo
{
    long long id;
    bool use_tounicode;
    int em_size;
    double space_width;
    double ascent, descent;
    bool is_type3;
    /*
     * As Type 3 fonts have a font matrix
     * a glyph of 1pt can be very large or very small
     * however it might not be true for other font formats such as ttf
     *
     * Therefore when we save a Type 3 font into ttf,
     * we have to scale the font to about 1,
     * then apply the scaling when using the font
     *
     * The scaling factor is stored as font_size_scale
     *
     * The value is 1 for other fonts
     */
    double font_size_scale;
};

struct HTMLTextState
{
    const FontInfo * font_info;
    double font_size;
    Color fill_color;
    Color stroke_color;
    double letter_space;
    double word_space;
    
    // relative to the previous state
    double vertical_align;
    
    // the offset cause by a single ' ' char
    double single_space_offset(void) const {
        double offset = word_space + letter_space;
        if(font_info->em_size != 0)
            offset += font_info->space_width * font_size;
        return offset;
    }
    // calculate em_size of this state
    double em_size(void) const {
        return font_size * (font_info->ascent - font_info->descent);
    }
};

struct HTMLLineState
{
    double x,y;
    double transform_matrix[4];
    // The page-cope char index(in drawing order) of the first char in this line.
    int first_char_index;
    // A function to determine whether a char is covered at a given index.
    std::function<bool(int)> is_char_covered;

    HTMLLineState(): first_char_index(-1) { }
};

struct HTMLClipState
{
    double xmin, xmax, ymin, ymax;
};

} // namespace pdf2htmlEX 

#endif //HTMLSTATE_H__
