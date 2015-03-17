/*
 * text.cc
 *
 * Handling text & font, and relative stuffs
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */


#include <algorithm>

#include "HTMLRenderer.h"

#include "util/namespace.h"
#include "util/unicode.h"

//#define HR_DEBUG(x)  (x)
#define HR_DEBUG(x)

namespace pdf2htmlEX {

using std::all_of;
using std::cerr;
using std::endl;

void HTMLRenderer::drawString(GfxState * state, GooString * s)
{
    if(s->getLength() == 0)
        return;

    auto font = state->getFont();
    double cur_letter_space = state->getCharSpace();
    double cur_word_space   = state->getWordSpace();
    double cur_horiz_scaling = state->getHorizScaling();


    // Writing mode fonts and Type 3 fonts are rendered as images
    // I don't find a way to display writing mode fonts in HTML except for one div for each character, which is too costly
    // For type 3 fonts, due to the font matrix, still it's hard to show it on HTML
    if( (font == nullptr) 
        || (font->getWMode())
        || ((font->getType() == fontType3) && (!param.process_type3))
      )
    {
        return;
    }

    // see if the line has to be closed due to state change
    check_state_change(state);
    prepare_text_line(state);

    // Now ready to output
    // get the unicodes
    char *p = s->getCString();
    int len = s->getLength();

    //accumulated displacement of chars in this string, in text object space
    double dx = 0;
    double dy = 0;
    //displacement of current char, in text object space, including letter space but not word space.
    double ddx, ddy;
    //advance of current char, in glyph space
    double ax, ay;
    //origin of current char, in glyph space
    double ox, oy;

    int uLen;

    CharCode code;
    Unicode *u = nullptr;

    HR_DEBUG(printf("HTMLRenderer::drawString:len=%d\n", len));

    while (len > 0) 
    {
        auto n = font->getNextChar(p, len, &code, &u, &uLen, &ax, &ay, &ox, &oy);
        HR_DEBUG(printf("HTMLRenderer::drawString:unicode=%lc(%d)\n", (wchar_t)u[0], u[0]));

        if(!(equal(ox, 0) && equal(oy, 0)))
        {
            cerr << "TODO: non-zero origins" << endl;
        }
        ddx = ax * cur_font_size + cur_letter_space;
        ddy = ay * cur_font_size;
        tracer.draw_char(state, dx, dy, ax, ay);

        bool is_space = false;
        if (n == 1 && *p == ' ')
        {
            /*
             * is_space indicates an ASCII SPACE before decoding.
             * This is by standard - only such characters are affected by "word space".
             * However some PDF will use ' ' as a normal encoding slot,
             * such that it will be mapped to other unicodes.
             */
            is_space = true;
        }

        if((param.decompose_ligature) && (uLen > 1) && all_of(u, u+uLen, isLegalUnicode))
        {
            html_text_page.get_cur_line()->append_unicodes(u, uLen, ddx);
        }
        else
        {
            Unicode uu;
            if(cur_text_state.font_info->use_tounicode)
            {
                uu = check_unicode(u, uLen, code, font);
            }
            else
            {
                uu = unicode_from_font(code, font);
            }
            // Mapping to unicode may be wrong in some PDFs, and uu == ' ' is actually a visible character.
            // In that case, when space_as_offset is on, we will simply ignore that character.
            if((param.space_as_offset) && (uu == ' '))
            {
                html_text_page.get_cur_line()->append_padding_char();
                // ignore horiz_scaling, as it has been merged into CTM
                double offset = ax * cur_font_size + cur_letter_space + (is_space ? cur_word_space : 0);
                html_text_page.get_cur_line()->append_offset(offset * draw_text_scale);
            }
            else
            {
                html_text_page.get_cur_line()->append_unicodes(&uu, 1, ddx);
                /*
                 * In PDF, word_space is appended if (n == 1 and *p = ' ')
                 * but in HTML, word_space is appended if (uu == ' ')
                 */
                int space_count = (is_space ? 1 : 0) - ((uu == ' ') ? 1 : 0);
                if(space_count != 0)
                {
                    html_text_page.get_cur_line()->append_offset(cur_word_space * draw_text_scale * space_count);
                }
            }
        }

        dx += ddx * cur_horiz_scaling;
        dy += ddy;
        if (is_space)
            dx += cur_word_space * cur_horiz_scaling;

        p += n;
        len -= n;
    }

    cur_tx += dx;
    cur_ty += dy;
        
    draw_tx += dx;
    draw_ty += dy;
}

bool HTMLRenderer::is_char_covered(int index)
{
    auto covered = covered_text_detector.get_chars_covered();
    if (index < 0 || index >= (int)covered.size())
    {
        std::cerr << "Warning: HTMLRenderer::is_char_covered: index out of bound: "
                << index << ", size: " << covered.size() <<endl;
        return false;
    }
    return covered[index];
}

} // namespace pdf2htmlEX
