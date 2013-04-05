/*
 * text.cc
 *
 * Handling text & font, and relative stuffs
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */


#include <algorithm>

#include "HTMLRenderer.h"
#include "TextLineBuffer.h"
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

    // Writing mode fonts and Type 3 fonts are rendered as images
    // I don't find a way to display writing mode fonts in HTML except for one div for each character, which is too costly
    // For type 3 fonts, due to the font matrix, still it's hard to show it on HTML
    if( (font == nullptr) 
        || (font->getWMode())
        || (font->getType() == fontType3)
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
    
    // current clipping rect (in device space)
    double xMin, yMin, xMax, yMax;
    state->getClipBBox(&xMin, &yMin, &xMax, &yMax);
    
    // current position (in device space)
    double x, y;
    state->transform(state->getCurX(), state->getCurY(), &x, &y);
    
    // total delta of the string
    double total_dx = 0;
    double total_dy = 0;
    
    while (len > 0) 
    {
        CharCode code; Unicode *u; int uLen;
        double char_dx, char_dy, origin_x, origin_y;
        auto n = font->getNextChar(p, len, &code, &u, &uLen, &char_dx, &char_dy, &origin_x, &origin_y);
        
        if(!(equal(origin_x, 0) && equal(origin_y, 0)))
        {
            cerr << "TODO: non-zero origins" << endl;
        }
        
        // adjust deltas for char space, word space, and horizontal scaling
        // source: PSOutputDev::drawString
        double dx = char_dx;
        double dy = char_dy;
        
        bool is_space = false;
        
        dx *= state->getFontSize();
        dy *= state->getFontSize();
        
        if (font->getWMode())
        {
            dy += state->getCharSpace();
            if (n == 1 && *p == ' ')
            {
                dy += state->getWordSpace();
                is_space = true;
            }
        }
        else
        {
            dx += state->getCharSpace();
            if (n == 1 && *p == ' ')
            {
                dx += state->getWordSpace();
                is_space = true;
            }
        }
        dx *= state->getHorizScaling();
        
        // transform from text space to device space
        double dev_total_dx, dev_total_dy;
        state->textTransformDelta(total_dx, total_dy, &dev_total_dx, &dev_total_dy);
        state->transformDelta(dev_total_dx, dev_total_dy, &dev_total_dx, &dev_total_dy);
        dev_total_dx *= text_zoom_factor();
        dev_total_dy *= text_zoom_factor();
        
        double dev_dx, dev_dy;
        state->textTransformDelta(dx, dy, &dev_dx, &dev_dy);
        state->transformDelta(dev_dx, dev_dy, &dev_dx, &dev_dy);
        dev_dx *= text_zoom_factor();
        dev_dy *= text_zoom_factor();
        
        // check if char is entirely outside the clipping rect
        if(x + dev_total_dx + dev_dx < xMin || x + dev_total_dx > xMax ||
           y + dev_total_dy + dev_dy < yMin || y + dev_total_dy > yMax)
        {
            // ignore horiz_scaling, as it's merged in CTM
            text_line_buf->append_offset((char_dx * state->getFontSize() + state->getCharSpace()) * draw_text_scale);
        }
        else if(is_space && (param->space_as_offset))
        {
            // ignore horiz_scaling, as it's merged in CTM
            text_line_buf->append_offset((char_dx * state->getFontSize() + state->getCharSpace() + state->getWordSpace()) * draw_text_scale); 
        }
        else
        {
            if((param->decompose_ligature) && (uLen > 1) && all_of(u, u+uLen, isLegalUnicode))
            {
                text_line_buf->append_unicodes(u, uLen);
            }
            else
            {
                if(cur_font_info->use_tounicode)
                {
                    Unicode uu = check_unicode(u, uLen, code, font);
                    text_line_buf->append_unicodes(&uu, 1);
                }
                else
                {
                    Unicode uu = unicode_from_font(code, font);
                    text_line_buf->append_unicodes(&uu, 1);
                }
            }
        }
        
        total_dx += dx;
        total_dy += dy;
        
        p += n;
        len -= n;
    }
    
    cur_tx += total_dx;
    cur_ty += total_dx;
        
    draw_tx += total_dx;
    draw_ty += total_dx;
}

} // namespace pdf2htmlEX
