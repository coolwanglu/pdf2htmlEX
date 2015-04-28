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

    page.chars.clear();
    page.words.clear();
    page.segments.clear();

    page.chars.reserve(preprocessor.get_char_count(pageNum));

    cur_word = nullptr;
    cur_segment = nullptr;

    cur_tx = 0;
    cur_ty = 0;
    cur_char_num = 0;

    begin_segment();
    begin_word();
}

void TextPageBuilder::endPage()
{
    end_segment();
    delete cur_word;
    cur_word = nullptr;
    delete cur_segment;
    cur_segment = nullptr;
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
    new_segment();
    updateTextPos(state);
}
void TextPageBuilder::updateRise(GfxState * state)
{
    new_word();
}
void TextPageBuilder::updateTextPos(GfxState * state) 
{
    new_word();
    cur_tx = state->getLineX(); 
    cur_ty = state->getLineY(); 
}
void TextPageBuilder::updateTextShift(GfxState * state, double shift) 
{
    new_word();
    cur_tx -= shift * 0.001 * state->getFontSize() * state->getHorizScaling();
}
void TextPageBuilder::updateFont(GfxState * state) 
{
    new_segment();
}
void TextPageBuilder::updateCTM(GfxState * state, double m11, double m12, double m21, double m22, double m31, double m32) 
{
    new_segment();
    //tracer.update_ctm(state, m11, m12, m21, m22, m31, m32);
}
void TextPageBuilder::updateTextMat(GfxState * state) 
{
    new_segment();
}
void TextPageBuilder::updateHorizScaling(GfxState * state)
{
    new_segment();
}
void TextPageBuilder::updateCharSpace(GfxState * state)
{
    new_segment();
}
void TextPageBuilder::updateWordSpace(GfxState * state)
{
    new_segment();
}
void TextPageBuilder::updateRender(GfxState * state) 
{
    new_segment();
}
void TextPageBuilder::updateFillColorSpace(GfxState * state) 
{
    new_segment();
}
void TextPageBuilder::updateStrokeColorSpace(GfxState * state) 
{
    new_segment();
}
void TextPageBuilder::updateFillColor(GfxState * state) 
{
    new_segment();
}
void TextPageBuilder::updateStrokeColor(GfxState * state) 
{
    new_segment();
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

    // Writing mode fonts are rendered as images
    // I don't find a way to display writing mode fonts in HTML except for one div for each character, which is too costly
    if( (font == nullptr) 
        || (font->getWMode())
        || ((font->getType() == fontType3) && (!param.process_type3))
      )
    {
        cur_char_num += s->getLength();
        return;
    }

    char *s_ptr = s->getCString();
    int s_len = s->getLength();

    CharCode code;

    while (s_len > 0) 
    {
        TextChar * cur_char = new TextChar();

        cur_char->order_number = cur_char_num;
        ++cur_char_num;

        cur_char->x = cur_tx;
        cur_char->y = cur_ty;

        Unicode *u_ptr = nullptr;
        int u_len;
        double ox, oy;

        int char_consumed = font->getNextChar(s_ptr, s_len, 
                                              &cur_char->code, 
                                              &u_ptr, &u_len, 
                                              &cur_char->dx, &cur_char->dy, 
                                              &ox, &oy);
        if(!(equal(ox, 0) && equal(oy, 0)))
            log_once("TODO: non-zero origins, please file an issue on GitHub!");

        cur_char->dx *= state->getFontSize();
        cur_char->dy *= state->getFontSize();
        cur_char->width = cur_char->dx + state->getCharSpace();
        if (char_consumed == 1 && *s_ptr == ' ') 
            cur_char->width += state->getWordSpace();

        // TODO: bbox for type 3 fonts
        // TODO: fix bbox for negative font sizes
        cur_char->bbox[0] = cur_tx; 
        cur_char->bbox[1] = cur_ty + font->getDescent() * state->getFontSize(); 
        cur_char->bbox[2] = cur_tx + cur_char->dx; 
        cur_char->bbox[3] = cur_ty + cur_char->dy + font->getAscent() * state->getFontSize(); 

        cur_char->unicodes.assign(u_ptr, u_ptr+u_len);

        cur_tx += cur_char->width;
        cur_ty += cur_char->dy;

        s_ptr += char_consumed;
        s_len -= char_consumed;

        cur_word->push_back(cur_char);
    }
}

void TextPageBuilder::begin_segment()
{
    if(!cur_segment)
        cur_segment = new TextSegment();
}

void TextPageBuilder::end_segment()
{
    end_word();
    if(cur_segment && !cur_segment->empty())
    {
        cur_segment->style = cur_style;
        page.push_back(cur_segment);
        cur_segment = nullptr;
    }
}

void TextPageBuilder::begin_word()
{
    if(!cur_word)
        cur_word = new TextWord();
}

void TextPageBuilder::end_word()
{
    if(cur_word && !cur_word->empty())
    {
        cur_segment.push_back(cur_word);
        cur_word = nullptr;
    }
}

void TextPageBuilder::copy_state(GfxState * state)
{
    auto & style = cur_segment->style;
    // TODO

}


} //namespace pdf2htmlEX
