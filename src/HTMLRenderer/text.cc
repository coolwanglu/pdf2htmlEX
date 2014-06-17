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

namespace pdf2htmlEX {

using std::all_of;
using std::cerr;
using std::endl;

void HTMLRenderer::drawString(GfxState * state, GooString * s)
{
    if(s->getLength() == 0)
        return;

    auto font = state->getFont();
    // unscaled
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

    double dx = 0;
    double dy = 0;
    double dx1,dy1;
    double ox, oy;

    int nChars = 0;
    int nSpaces = 0;
    int uLen;

    CharCode code;
    Unicode *u = nullptr;

    while (len > 0) 
    {
        auto n = font->getNextChar(p, len, &code, &u, &uLen, &dx1, &dy1, &ox, &oy);

        if(!(equal(ox, 0) && equal(oy, 0)))
        {
            cerr << "TODO: non-zero origins" << endl;
        }

        add_char_bbox(state, dx, dy, dx1, dy1);

        bool is_space = false;
        if (n == 1 && *p == ' ') 
        {
            /*
             * This is by standard
             * however some PDF will use ' ' as a normal encoding slot
             * such that it will be mapped to other unicodes
             * In that case, when space_as_offset is on, we will simply ignore that character...
             *
             * Checking mapped unicode may or may not work
             * There are always ugly PDF files with no useful info at all.
             */
            is_space = true;
            ++nSpaces;
        }
        
        if(is_space && (param.space_as_offset))
        {
            // ignore horiz_scaling, as it has been merged into CTM
            html_text_page.get_cur_line()->append_offset((dx1 * cur_font_size + cur_letter_space + cur_word_space) * draw_text_scale); 
        }
        else
        {
            if((param.decompose_ligature) && (uLen > 1) && all_of(u, u+uLen, isLegalUnicode))
            {
                // TODO: why multiply cur_horiz_scaling here?
                html_text_page.get_cur_line()->append_unicodes(u, uLen, (dx1 * cur_font_size + cur_letter_space) * cur_horiz_scaling);
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
                // TODO: why multiply cur_horiz_scaling here?
                html_text_page.get_cur_line()->append_unicodes(&uu, 1, (dx1 * cur_font_size + cur_letter_space) * cur_horiz_scaling);
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

        dx += dx1;
        dy += dy1;

        ++nChars;
        p += n;
        len -= n;
    }

    // horiz_scaling is merged into ctm now, 
    // so the coordinate system is ugly
    // TODO: why multiply cur_horiz_scaling here
    dx = (dx * cur_font_size + nChars * cur_letter_space + nSpaces * cur_word_space) * cur_horiz_scaling;
    
    dy *= cur_font_size;

    cur_tx += dx;
    cur_ty += dy;
        
    draw_tx += dx;
    draw_ty += dy;
}

void HTMLRenderer::add_char_bbox(GfxState *state, double x, double y, double ax, double ay)
{
    if (!param.process_covered_text)
        return;

    Matrix tm_ctm, tm, itm;
    memcpy(tm_ctm.m, this->cur_text_tm, sizeof(tm_ctm.m));
    memcpy(tm.m, state->getTextMat(), sizeof(tm.m));
    double fs = state->getFontSize();

    double cx = state->getCurX(), cy = state->getCurY(),
            ry = state->getRise(), h = state->getHorizScaling();

    //cx and cy has been transformed by text matrix, we need to reverse them.
    tm.invertTo(&itm);
    double char_cx, char_cy;
    itm.transform(cx, cy, &char_cx, &char_cy);

    //TODO Vertical? Currently vertical/type3 chars are treated as non-chars.
    double tchar[6] {fs * h, 0, 0, fs, char_cx + x, char_cy + y + ry};

    double tfinal[6];
    tm_multiply(tfinal, tm_ctm.m, tchar);

    auto font = state->getFont();
    double bbox[4] {0, 0, ax, ay};
    double desc = font->getDescent(), asc = font->getAscent();
    if (font->getWMode() == 0)
    {
        bbox[1] += desc;
        bbox[3] += asc;
    }
    else
    {//TODO Vertical?
    }
    tm_transform_bbox(tfinal, bbox);
    covered_text_handler.add_char_bbox(bbox);
}

} // namespace pdf2htmlEX
