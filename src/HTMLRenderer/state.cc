/*
 * state.cc
 *
 * track current state
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

/*
 * TODO
 * optimize lines using nested <span> (reuse classes)
 */

#include <cmath>
#include <algorithm>

#include "HTMLRenderer.h"
#include "TextLineBuffer.h"
#include "util/namespace.h"
#include "util/math.h"

namespace pdf2htmlEX {

using std::max;
using std::abs;

void HTMLRenderer::updateAll(GfxState * state) 
{ 
    all_changed = true; 
    updateTextPos(state);
}
void HTMLRenderer::updateRise(GfxState * state)
{
    rise_changed = true;
}
void HTMLRenderer::updateTextPos(GfxState * state) 
{
    text_pos_changed = true;
    cur_tx = state->getLineX(); 
    cur_ty = state->getLineY(); 
}
void HTMLRenderer::updateTextShift(GfxState * state, double shift) 
{
    text_pos_changed = true;
    cur_tx -= shift * 0.001 * state->getFontSize() * state->getHorizScaling(); 
}
void HTMLRenderer::updateFont(GfxState * state) 
{
    font_changed = true; 
}
void HTMLRenderer::updateCTM(GfxState * state, double m11, double m12, double m21, double m22, double m31, double m32) 
{
    ctm_changed = true; 
}
void HTMLRenderer::updateTextMat(GfxState * state) 
{
    text_mat_changed = true; 
}
void HTMLRenderer::updateHorizScaling(GfxState * state)
{
    hori_scale_changed = true;
}
void HTMLRenderer::updateCharSpace(GfxState * state)
{
    letter_space_changed = true;
}

void HTMLRenderer::updateWordSpace(GfxState * state)
{
    word_space_changed = true;
}
void HTMLRenderer::updateRender(GfxState * state) 
{
    // currently Render is traced for color only
    // might need something like render_changed later
    fill_color_changed = true; 
    stroke_color_changed = true; 
}
void HTMLRenderer::updateFillColorSpace(GfxState * state) 
{
    fill_color_changed = true; 
}
void HTMLRenderer::updateStrokeColorSpace(GfxState * state) 
{
    stroke_color_changed = true; 
}
void HTMLRenderer::updateFillColor(GfxState * state) 
{
    fill_color_changed = true; 
}
void HTMLRenderer::updateStrokeColor(GfxState * state) 
{
    stroke_color_changed = true; 
}
void HTMLRenderer::reset_state()
{
    draw_text_scale = 1.0;

    cur_font_info = install_font(nullptr);
    cur_font_size = draw_font_size = 0;
    cur_fs_id = install_font_size(cur_font_size);
    
    memcpy(cur_text_tm, ID_MATRIX, sizeof(cur_text_tm));
    memcpy(draw_text_tm, ID_MATRIX, sizeof(draw_text_tm));
    cur_ttm_id = install_transform_matrix(draw_text_tm);

    cur_letter_space = cur_word_space = 0;
    cur_ls_id = install_letter_space(cur_letter_space);
    cur_ws_id = install_word_space(cur_word_space);

    cur_fill_color.r = cur_fill_color.g = cur_fill_color.b = 0;
    cur_fill_color_id = install_fill_color(&cur_fill_color);
    cur_has_fill = true;

    cur_stroke_color.r = cur_stroke_color.g = cur_stroke_color.b = 0;
    cur_stroke_color_id = install_stroke_color(&cur_stroke_color);
    cur_has_stroke = false;

    cur_rise = 0;
    cur_rise_id = install_rise(cur_rise);

    cur_tx = cur_ty = 0;
    draw_tx = draw_ty = 0;

    reset_state_change();
    all_changed = true;
}
void HTMLRenderer::reset_state_change()
{
    all_changed = false;

    rise_changed = false;
    text_pos_changed = false;

    font_changed = false;
    ctm_changed = false;
    text_mat_changed = false;
    hori_scale_changed = false;

    letter_space_changed = false;
    word_space_changed = false;

    fill_color_changed = false;
    stroke_color_changed = false;
}
void HTMLRenderer::check_state_change(GfxState * state)
{
    // DEPENDENCY WARNING
    // don't adjust the order of state checking 
    
    new_line_state = NLS_NONE;

    bool need_recheck_position = false;
    bool need_rescale_font = false;
    bool draw_text_scale_changed = false;

    // text position
    // we've been tracking the text position positively in the update*** functions
    if(all_changed || text_pos_changed)
    {
        need_recheck_position = true;
    }

    // font name & size
    if(all_changed || font_changed)
    {
        const FontInfo * new_font_info = install_font(state->getFont());

        if(!(new_font_info->id == cur_font_info->id))
        {
            // Currently Type 3 font are shown hidden in HTML, with default fonts (at viewers' machine)
            // The width of the text displayed is likely to be wrong
            // So we will create separate (absolute positioned) blocks for them, such that it won't affect other text
            if(new_font_info->is_type3 || cur_font_info->is_type3)
            {
                new_line_state = max<NewLineState>(new_line_state, NLS_DIV);
            }
            else
            {
                new_line_state = max<NewLineState>(new_line_state, NLS_SPAN);
            }
            cur_font_info = new_font_info;
        }

        double new_font_size = state->getFontSize();
        if(!equal(cur_font_size, new_font_size))
        {
            need_rescale_font = true;
            cur_font_size = new_font_size;
        }
    }  

    // backup the current ctm for need_recheck_position
    double old_ctm[6];
    memcpy(old_ctm, cur_text_tm, sizeof(old_ctm));

    // ctm & text ctm & hori scale
    if(all_changed || ctm_changed || text_mat_changed || hori_scale_changed)
    {
        double new_ctm[6];

        const double * m1 = state->getCTM();
        const double * m2 = state->getTextMat();
        double hori_scale = state->getHorizScaling();

        new_ctm[0] = (m1[0] * m2[0] + m1[2] * m2[1]) * hori_scale;
        new_ctm[1] = (m1[1] * m2[0] + m1[3] * m2[1]) * hori_scale;
        new_ctm[2] = m1[0] * m2[2] + m1[2] * m2[3];
        new_ctm[3] = m1[1] * m2[2] + m1[3] * m2[3];
        new_ctm[4] = m1[0] * m2[4] + m1[2] * m2[5] + m1[4]; 
        new_ctm[5] = m1[1] * m2[4] + m1[3] * m2[5] + m1[5];
        //new_ctm[4] = new_ctm[5] = 0;

        if(!tm_equal(new_ctm, cur_text_tm))
        {
            need_recheck_position = true;
            need_rescale_font = true;
            memcpy(cur_text_tm, new_ctm, sizeof(cur_text_tm));
        }
    }

    // draw_text_tm, draw_text_scale
    // depends: font size & ctm & text_ctm & hori scale
    if(need_rescale_font)
    {
        double new_draw_text_tm[6];
        memcpy(new_draw_text_tm, cur_text_tm, sizeof(new_draw_text_tm));

        double new_draw_text_scale = 1.0/text_scale_factor2 * hypot(new_draw_text_tm[2], new_draw_text_tm[3]);

        double new_draw_font_size = cur_font_size;
        if(is_positive(new_draw_text_scale))
        {
            new_draw_font_size *= new_draw_text_scale;
            for(int i = 0; i < 4; ++i)
                new_draw_text_tm[i] /= new_draw_text_scale;
        }
        else
        {
            new_draw_text_scale = 1.0;
        }

        if(!is_positive(new_draw_font_size))
        {
            // Page is flipped and css can't handle it.
            new_draw_font_size = -new_draw_font_size;

            for(int i = 0; i < 4; ++i)
                new_draw_text_tm[i] *= -1;
        }

        if(!(equal(new_draw_text_scale, draw_text_scale)))
        {
            draw_text_scale_changed = true;
            draw_text_scale = new_draw_text_scale;
        }

        if(!(equal(new_draw_font_size, draw_font_size)))
        {
            new_line_state = max<NewLineState>(new_line_state, NLS_SPAN);
            draw_font_size = new_draw_font_size;
            cur_fs_id = install_font_size(draw_font_size);
        }
        if(!(tm_equal(new_draw_text_tm, draw_text_tm, 4)))
        {
            new_line_state = max<NewLineState>(new_line_state, NLS_DIV);
            memcpy(draw_text_tm, new_draw_text_tm, sizeof(draw_text_tm));
            cur_ttm_id = install_transform_matrix(draw_text_tm);
        }
    }

    // see if we can merge with the current line
    // depends: rise & text position & transformation
    if(need_recheck_position)
    {
        // try to transform the old origin under the new TM
        /*
         * OldTM * (draw_tx, draw_ty, 1)^T = CurTM * (draw_tx + dx, draw_ty + dy, 1)^T
         *
         * OldTM[4] = CurTM[0] * dx + CurTM[2] * dy + CurTM[4] 
         * OldTM[5] = CurTM[1] * dx + CurTM[3] * dy + CurTM[5] 
         *
         * We just care if we can map the origin y to the same new y
         * So just let dy = cur_y - old_y, and try to solve dx
         *
         * TODO, writing mode, set dx and solve dy
         */

        bool merged = false;
        if(tm_equal(old_ctm, cur_text_tm, 4))
        {
            double dy = cur_ty - draw_ty;
            double tdx = old_ctm[4] - cur_text_tm[4] - cur_text_tm[2] * dy;
            double tdy = old_ctm[5] - cur_text_tm[5] - cur_text_tm[3] * dy;

            if(equal(cur_text_tm[0] * tdy, cur_text_tm[1] * tdx))
            {
                if(is_positive(cur_text_tm[0]))
                {
                    draw_tx += tdx / cur_text_tm[0];
                    draw_ty += dy;
                    merged = true;
                }
                else if (is_positive(cur_text_tm[1]))
                {
                    draw_tx += tdy / cur_text_tm[1];
                    draw_ty += dy;
                    merged = true;
                }
                else
                {
                    if((equal(tdx,0)) && (equal(tdy,0)))
                    {
                        // free
                        draw_tx = cur_tx;
                        draw_ty += dy;
                        merged = true;
                    }
                    // else fail
                }
            }
            //else no solution
        }
        // else force new line

        if(!merged)
        {
            new_line_state = max<NewLineState>(new_line_state, NLS_DIV);
        }
    }

    // letter space
    // depends: draw_text_scale
    if(all_changed || letter_space_changed || draw_text_scale_changed)
    {
        double new_letter_space = state->getCharSpace();
        if(!equal(cur_letter_space, new_letter_space))
        {
            new_line_state = max<NewLineState>(new_line_state, NLS_SPAN);
            cur_letter_space = new_letter_space;
            cur_ls_id = install_letter_space(cur_letter_space * draw_text_scale);
        }
    }

    // word space
    // depends draw_text_scale
    if(all_changed || word_space_changed || draw_text_scale_changed)
    {
        double new_word_space = state->getWordSpace();
        if(!equal(cur_word_space, new_word_space))
        {
            new_line_state = max<NewLineState>(new_line_state, NLS_SPAN);
            cur_word_space = new_word_space;
            cur_ws_id = install_word_space(cur_word_space * draw_text_scale);
        }
    }

    // color
    if(all_changed || fill_color_changed || stroke_color_changed)
    {
        /*
         * PDF Spec. Table 106 –  Text rendering modes
         */

        static const char FILL[8]   = { true, false, true, false, true, false, true, false };
        static const char STROKE[8] = { false, true, true, false, false, true, true, false };
        
        int idx = state->getRender();
        assert((idx >= 0) && (idx < 8));
        bool is_filled = FILL[idx];
        bool is_stroked = STROKE[idx];
        
        
        // fill
        if(is_filled)
        {
            GfxRGB new_color;
            state->getFillRGB(&new_color);
        
            if(!cur_has_fill
               || (!GfxRGB_equal()(new_color, cur_fill_color))
              )
            {
                new_line_state = max<NewLineState>(new_line_state, NLS_SPAN);
                cur_fill_color = new_color;
                cur_fill_color_id = install_fill_color(&new_color);
            }
        }
        else
        {
            cur_fill_color_id = install_fill_color(nullptr);
        }
        cur_has_fill = is_filled;
        
        // stroke
        if(is_stroked)
        {
            GfxRGB new_color;
            state->getStrokeRGB(&new_color);

            if(!cur_has_stroke 
                    || (!GfxRGB_equal()(new_color, cur_stroke_color))
              )
            {
                new_line_state = max<NewLineState>(new_line_state, NLS_SPAN);
                cur_stroke_color = new_color;
                cur_stroke_color_id = install_stroke_color(&new_color);
            }
        }
        else
        {
            cur_stroke_color_id = install_stroke_color(nullptr);
        }
        cur_has_stroke = is_stroked;
    }

    // rise
    // depends draw_text_scale
    if(all_changed || rise_changed || draw_text_scale_changed)
    {
        double new_rise = state->getRise();
        if(!equal(cur_rise, new_rise))
        {
            new_line_state = max<NewLineState>(new_line_state, NLS_SPAN);
            cur_rise = new_rise;
            cur_rise_id = install_rise(new_rise * draw_text_scale);
        }
    }

    reset_state_change();
}

void HTMLRenderer::prepare_text_line(GfxState * state)
{
    if(!line_opened)
    {
        new_line_state = NLS_DIV;
    }
    
    if(new_line_state == NLS_DIV)
    {
        close_text_line();

        text_line_buf->reset(state);

        //resync position
        draw_ty = cur_ty;
        draw_tx = cur_tx;
    }
    else
    {
        // align horizontal position
        // try to merge with the last line if possible
        double target = (cur_tx - draw_tx) * draw_text_scale;
        if(abs(target) < param->h_eps)
        {
            // ignore it
        }
        else
        {
            text_line_buf->append_offset(target);
            draw_tx += target / draw_text_scale;
        }
    }

    if(new_line_state != NLS_NONE)
    {
        text_line_buf->append_state();
    }

    line_opened = true;
}

void HTMLRenderer::close_text_line()
{
    if(line_opened)
    {
        line_opened = false;
        text_line_buf->flush();
    }
}

} //namespace pdf2htmlEX
