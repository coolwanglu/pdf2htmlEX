/*
 * TextPageBuilder.cc
 * Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>
 */

#include <cstring>

#include "TextPageBuilder.h"

namespace pdf2htmlEX {

void TextPageBuilder::setDefaultCTM(double * ctm)
{
    memcpy(page.default_ctm, ctm, sizeof(page.default_ctm));
}

void TextPageBuilder::startPage(int pageNum, GfxState *state, XRef * xref)
{
    page.num = pageNum;
    page.width = state->getPageWidth();
    page.height = state->getPageHeight();
}

void TextPageBuilder::endPage()
{
    end_segment();
    // run passes
}

void TextPageBuilder::restoreState(GfxState * state)
{
    updateAll(state);
}

void TextPageBuilder::saveState(GfxState * state)
{
}

void TextPageBuilder::updateAll(GfxState * state) 
{ 
    new_segment(state, true);
    updateTextPos(state);
}
void TextPageBuilder::updateRise(GfxState * state)
{
    new_word(state);
}
void TextPageBuilder::updateTextPos(GfxState * state) 
{
    new_word(state);
    cur_tx = state->getLineX(); 
    cur_ty = state->getLineY(); 
}
void TextPageBuilder::updateTextShift(GfxState * state, double shift) 
{
    new_word(state);
    cur_tx -= shift * 0.001 * state->getFontSize() * state->getHorizScaling();
}
void TextPageBuilder::updateFont(GfxState * state) 
{
    new_segment(state);
}
void TextPageBuilder::updateCTM(GfxState * state, double m11, double m12, double m21, double m22, double m31, double m32) 
{
    new_segment(state);
    //tracer.update_ctm(state, m11, m12, m21, m22, m31, m32);
}
void TextPageBuilder::updateTextMat(GfxState * state) 
{
    new_segment(state);
}
void TextPageBuilder::updateHorizScaling(GfxState * state)
{
    new_segment(state);
}
void TextPageBuilder::updateCharSpace(GfxState * state)
{
    new_segment(state);
}
void TextPageBuilder::updateWordSpace(GfxState * state)
{
    new_segment(state);
}
void TextPageBuilder::updateRender(GfxState * state) 
{
    new_segment(state);
}
void TextPageBuilder::updateFillColorSpace(GfxState * state) 
{
    new_segment(state);
}
void TextPageBuilder::updateStrokeColorSpace(GfxState * state) 
{
    new_segment(state);
}
void TextPageBuilder::updateFillColor(GfxState * state) 
{
    new_segment(state);
}
void TextPageBuilder::updateStrokeColor(GfxState * state) 
{
    new_segment(state);
}
void TextPageBuilder::clip(GfxState * state)
{
    //tracer.clip(state);
}
void TextPageBuilder::eoClip(GfxState * state)
{
    //tracer.clip(state, true);
}
void TextPageBuilder::clipToStrokePath(GfxState * state)
{
    //tracer.clip_to_stroke_path(state);
}

void TextPageBuilder::drawString(GfxState * state, GooString * s)
{
    if(s->getLength() == 0)
        return;

    auto font = state->getFont();
    double cur_letter_space = state->getCharSpace();
    double cur_word_space   = state->getWordSpace();
    double cur_horiz_scaling = state->getHorizScaling();

    // Writing mode fonts are rendered as images
    // I don't find a way to display writing mode fonts in HTML except for one div for each character, which is too costly
    if( (font == nullptr) 
        || (font->getWMode())
        || ((font->getType() == fontType3) && (!param.process_type3))
      )
    {
        return;
    }

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

    while (len > 0) 
    {
        auto n = font->getNextChar(p, len, &code, &u, &uLen, &ax, &ay, &ox, &oy);

        if(!(equal(ox, 0) && equal(oy, 0)))
        {
            log_once("TODO: non-zero origins, please file an issue on GitHub!");
        }
        ddx = ax * cur_font_size + cur_letter_space;
        ddy = ay * cur_font_size;
        tracer.draw_char(state, dx, dy, ax, ay);

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
        }
        
        if(is_space && (param.space_as_offset))
        {
            html_text_page.get_cur_line()->append_padding_char();
            // ignore horiz_scaling, as it has been merged into CTM
            html_text_page.get_cur_line()->append_offset(ax * cur_font_size + cur_letter_space + cur_word_space);
        }
        else
        {
            if((param.decompose_ligature) && (uLen > 1) && none_of(u, u+uLen, is_illegal_unicode))
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
                html_text_page.get_cur_line()->append_unicodes(&uu, 1, ddx);
                /*
                 * In PDF, word_space is appended if (n == 1 and *p = ' ')
                 * but in HTML, word_space is appended if (uu == ' ')
                 */
                int space_count = (is_space ? 1 : 0) - ((uu == ' ') ? 1 : 0);
                if(space_count != 0)
                {
                    html_text_page.get_cur_line()->append_offset(cur_word_space * space_count);
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

    draw_tx += dx;
    draw_ty += dy;
}

bool TextPageBuilder::new_segment(GfxState * state, bool copy_state)
{
    if(!cur_segment)
    {
        cur_segment = new TextSegment();
        copy_state(state);
        return true;
    }

    if (copy_state)
        copy_state(state);
    return false;
}

void TextPageBuilder::end_segment()
{
    end_word();
    if(cur_segment && !cur_segment->empty())
    {
        page.push_back(*cur_segment);
        cur_segment = nullptr;
    }
}

void TextPageBuilder::begin_word(GfxState * state)
{
    if(!cur_word)
        cur_word = new TextWord();
}

void TextPageBuilder::end_word()
{
    if(cur_word && !cur_word->empty())
    {
        cur_segment->push_back(cur_word);
        cur_word = nullptr;
    }
}

void TextPageBuilder::copy_state(GfxState * state)
{
    auto & style = cur_segment->style;
    // TODO

}


} //namespace pdf2htmlEX
