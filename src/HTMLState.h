/*
 * Header file for HTMLState
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */
#ifndef HTMLSTATE_H__
#define HTMLSTATE_H__

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
};

struct HTMLState
{
    const FontInfo * font_info;
    double font_size;
    Color fill_color;
    Color stroke_color;
    double letter_space;
    double word_space;
    
    // relative to the previous state
    double vertical_align;
    
    double x,y;
    double transform_matrix[4];

    // the offset cause by a single ' ' char
    double single_space_offset(void) const {
        return word_space + letter_space + font_info->space_width * font_size;
    }
    // calculate em_size of this state
    double em_size(void) const {
        return font_size * (font_info->ascent - font_info->descent);
    }
};

} // namespace pdf2htmlEX 

#endif //HTMLSTATE_H__
