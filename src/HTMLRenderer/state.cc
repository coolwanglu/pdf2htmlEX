/*
 * state.cc
 *
 * track PDF states
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <cmath>
#include <algorithm>

#include "HTMLRenderer.h"

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
    tracer.update_ctm(state, m11, m12, m21, m22, m31, m32);
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
void HTMLRenderer::clip(GfxState * state)
{
    clip_changed = true;
    tracer.clip(state);
}
void HTMLRenderer::eoClip(GfxState * state)
{
    clip_changed = true;
    tracer.clip(state, true);
}
void HTMLRenderer::clipToStrokePath(GfxState * state)
{
    clip_changed = true;
    tracer.clip_to_stroke_path(state);
}
void HTMLRenderer::reset_state()
{
    inTransparencyGroup = 0;
    draw_text_scale = 1.0;

    cur_font_size = 0.0;
    
    memcpy(cur_text_tm, ID_MATRIX, sizeof(cur_text_tm));

    // reset html_state
    cur_text_state.font_info = install_font(nullptr);
    cur_text_state.font_size = 0;
    cur_text_state.fill_color.transparent = true;
    cur_text_state.stroke_color.transparent = true;
    cur_text_state.letter_space = 0;
    cur_text_state.word_space = 0;
    cur_text_state.vertical_align = 0;

    cur_line_state.x = 0;
    cur_line_state.y = 0;
    memcpy(cur_line_state.transform_matrix, ID_MATRIX, sizeof(cur_line_state.transform_matrix));

    cur_line_state.is_char_covered = [this](int index) { return is_char_covered(index);};

    cur_clip_state.xmin = 0;
    cur_clip_state.xmax = 0;
    cur_clip_state.ymin = 0;
    cur_clip_state.ymax = 0;

    cur_tx  = cur_ty  = 0;
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

    clip_changed = false;
}

template<class NewLineState>
void set_line_state(NewLineState & cur_ls, NewLineState new_ls)
{
    if(new_ls > cur_ls)
        cur_ls = new_ls;
}

void HTMLRenderer::check_state_change(GfxState * state)
{
    // DEPENDENCY WARNING
    // don't adjust the order of state checking 
    
    new_line_state = NLS_NONE;

    if(all_changed || clip_changed)
    {
        HTMLClipState new_clip_state;
        state->getClipBBox(&new_clip_state.xmin, &new_clip_state.ymin, &new_clip_state.xmax, &new_clip_state.ymax);
        if(!(equal(cur_clip_state.xmin, new_clip_state.xmin)
                    && equal(cur_clip_state.xmax, new_clip_state.xmax)
                    && equal(cur_clip_state.ymin, new_clip_state.ymin)
                    && equal(cur_clip_state.ymax, new_clip_state.ymax)))
        {
            cur_clip_state = new_clip_state;
            set_line_state(new_line_state, NLS_NEWCLIP);
        }
    }

    bool need_recheck_position = false;
    bool need_rescale_font = false;
    bool draw_text_scale_changed = false;

    // save current info for later use
    auto old_text_state = cur_text_state;
    auto old_line_state = cur_line_state;
    double old_tm[6];
    memcpy(old_tm, cur_text_tm, sizeof(old_tm));
    double old_draw_text_scale = draw_text_scale;

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

        if(!(new_font_info->id == cur_text_state.font_info->id))
        {
            // The width of the type 3 font text, if shown, is likely to be wrong
            // So we will create separate (absolute positioned) blocks for them, such that it won't affect other text
            if((new_font_info->is_type3 || cur_text_state.font_info->is_type3) && (!param.process_type3))
            {
                set_line_state(new_line_state, NLS_NEWLINE);
            }
            else
            {
                set_line_state(new_line_state, NLS_NEWSTATE);
            }
            cur_text_state.font_info = new_font_info;
        }

        /*
         * For Type 3 fonts, we need to take type3_font_size_scale into consideration
         */
        if((new_font_info->is_type3 || cur_text_state.font_info->is_type3) && param.process_type3)
            need_rescale_font = true;

        double new_font_size = state->getFontSize();
        if(!equal(cur_font_size, new_font_size))
        {
            need_rescale_font = true;
            cur_font_size = new_font_size;
        }
    }  

    // ctm & text ctm & hori scale & rise
    if(all_changed || ctm_changed || text_mat_changed || hori_scale_changed || rise_changed)
    {
        double new_text_tm[6];

        double m1[6];
        double m2[6];

        //the matrix with horizontal_scale and rise
        m1[0] = state->getHorizScaling();
        m1[3] = 1;
        m1[5] = state->getRise();
        m1[1] = m1[2] = m1[4] = 0;

        tm_multiply(m2, state->getCTM(), state->getTextMat()); 
        tm_multiply(new_text_tm, m2, m1);

        if(!tm_equal(new_text_tm, cur_text_tm))
        {
            need_recheck_position = true;
            need_rescale_font = true;
            memcpy(cur_text_tm, new_text_tm, sizeof(cur_text_tm));
        }
    }

    // draw_text_tm, draw_text_scale
    // depends: font size & ctm & text_ctm & hori scale & rise
    if(need_rescale_font)
    {
        /*
         * Rescale the font
         * If the font-size is 1, and the matrix is [10,0,0,10,0,0], we would like to change it to
         * font-size == 10 and matrix == [1,0,0,1,0,0], 
         * such that it will be easy and natural for web browsers
         */
        double new_draw_text_tm[6];
        memcpy(new_draw_text_tm, cur_text_tm, sizeof(new_draw_text_tm));

        // see how the tm (together with text_scale_factor2) would change the vector (0,1)
        double new_draw_text_scale = 1.0/text_scale_factor2 * hypot(new_draw_text_tm[2], new_draw_text_tm[3]);

        double new_draw_font_size = cur_font_size;

        if(is_positive(new_draw_text_scale))
        {
            // scale both font size and matrix 
            new_draw_font_size *= new_draw_text_scale;
            for(int i = 0; i < 4; ++i)
                new_draw_text_tm[i] /= new_draw_text_scale;
        }
        else
        {
            new_draw_text_scale = 1.0;
        }

        if(is_positive(-new_draw_font_size))
        {
            // CSS cannot handle flipped pages
            new_draw_font_size *= -1;

            for(int i = 0; i < 4; ++i)
                new_draw_text_tm[i] *= -1;
        }

        if(!(equal(new_draw_text_scale, draw_text_scale)))
        {
            draw_text_scale_changed = true;
            draw_text_scale = new_draw_text_scale;
        }

        if(!equal(new_draw_font_size, cur_text_state.font_size))
        {
            set_line_state(new_line_state, NLS_NEWSTATE);
            cur_text_state.font_size = new_draw_font_size;
        }

        if(!tm_equal(new_draw_text_tm, cur_line_state.transform_matrix, 4))
        {
            set_line_state(new_line_state, NLS_NEWLINE);
            memcpy(cur_line_state.transform_matrix, new_draw_text_tm, sizeof(cur_line_state.transform_matrix));
        }
    }

    // see if the new line is compatible with the current line with proper position shift
    // don't bother doing the heavy job when (new_line_state == NLS_NEWLINE)
    // depends: text position & transformation
    if(need_recheck_position && (new_line_state < NLS_NEWLINE))
    {
        // TM[4] and/or TM[5] have been changed
        // To find an offset (dx,dy), which would cancel the effect
        /*
         * CurTM * (cur_tx, cur_ty, 1)^T = OldTM * (draw_tx + dx, draw_ty + dy, 1)^T
         *
         * the first 4 elements of CurTM and OldTM should be the proportional
         * otherwise the following text cannot be parallel
         *
         * NOTE:
         * dx,dy are handled by the old state. so they should be multiplied by old_draw_text_scale
         */

        bool merged = false;
        double dx = 0;
        double dy = 0;
        if(tm_equal(old_line_state.transform_matrix, cur_line_state.transform_matrix, 4))
        {
            double det = old_tm[0] * old_tm[3] - old_tm[1] * old_tm[2];
            if(!equal(det, 0))
            {
                double lhs1 = cur_text_tm[0] * cur_tx + cur_text_tm[2] * cur_ty + cur_text_tm[4] - old_tm[0] * draw_tx - old_tm[2] * draw_ty - old_tm[4];
                double lhs2 = cur_text_tm[1] * cur_tx + cur_text_tm[3] * cur_ty + cur_text_tm[5] - old_tm[1] * draw_tx - old_tm[3] * draw_ty - old_tm[5];
                /*
                 * Now the equation system becomes
                 *
                 * lhs1 = OldTM[0] * dx + OldTM[2] * dy
                 * lhs2 = OldTM[1] * dx + OldTM[3] * dy
                 */

                double inverted[4];
                inverted[0] =  old_tm[3] / det;
                inverted[1] = -old_tm[1] / det;
                inverted[2] = -old_tm[2] / det;
                inverted[3] =  old_tm[0] / det;
                dx = inverted[0] * lhs1 + inverted[2] * lhs2;
                dy = inverted[1] * lhs1 + inverted[3] * lhs2;
                if(equal(dy, 0))
                {
                    // text on a same horizontal line, we can insert positive or negative x-offsets
                    merged = true;
                }
                else if(param.optimize_text)
                {
                    // otherwise we merge the lines only when
                    // - text are not shifted to the left too much
                    // - text are not moved too high or too low
                    if((dx * old_draw_text_scale) >= -param.space_threshold * old_text_state.em_size() - EPS)
                    {
                        double oldymin = old_text_state.font_info->descent * old_text_state.font_size;
                        double oldymax = old_text_state.font_info->ascent * old_text_state.font_size;
                        double ymin = dy * old_draw_text_scale + cur_text_state.font_info->descent * cur_text_state.font_size;
                        double ymax = dy * old_draw_text_scale + cur_text_state.font_info->ascent * cur_text_state.font_size;
                        if((ymin <= oldymax + EPS) && (ymax >= oldymin - EPS))
                        {
                            merged = true;
                        }
                    }
                }
            }
            //else no solution
        }
        // else: different rotation: force new line

        if(merged && !equal(state->getHorizScaling(), 0))
        {
            html_text_page.get_cur_line()->append_offset(dx * old_draw_text_scale / state->getHorizScaling());
            if(equal(dy, 0))
            {
                cur_text_state.vertical_align = 0;
            }
            else
            {
                cur_text_state.vertical_align = (dy * old_draw_text_scale);
                set_line_state(new_line_state, NLS_NEWSTATE);
            }
            draw_tx = cur_tx;
            draw_ty = cur_ty;
        }
        else
        {
            set_line_state(new_line_state, NLS_NEWLINE);
        }
    }
    else
    {
        // no vertical shift if no need to check position
        cur_text_state.vertical_align = 0;
    }

    // letter space
    // depends: draw_text_scale
    if(all_changed || letter_space_changed || draw_text_scale_changed)
    {
        double new_letter_space = state->getCharSpace() * draw_text_scale;
        if(!equal(new_letter_space, cur_text_state.letter_space))
        {
            cur_text_state.letter_space = new_letter_space;
            set_line_state(new_line_state, NLS_NEWSTATE);
        }
    }

    // word space
    // depends draw_text_scale
    if(all_changed || word_space_changed || draw_text_scale_changed)
    {
        double new_word_space = state->getWordSpace() * draw_text_scale;
        if(!equal(new_word_space, cur_text_state.word_space))
        {
            cur_text_state.word_space = new_word_space;
            set_line_state(new_line_state, NLS_NEWSTATE);
        }
    }

    // fill color
    if((!(param.fallback)) && (all_changed || fill_color_changed))
    {
        // * PDF Spec. Table 106 –Text rendering modes
        static const char FILL[8] = { true, false, true, false, true, false, true, false };
        
        int idx = state->getRender();
        assert((idx >= 0) && (idx < 8));
        Color new_fill_color;
        if(FILL[idx])
        {
            new_fill_color.transparent = false;
            state->getFillRGB(&new_fill_color.rgb);
        }
        else
        {
            new_fill_color.transparent = true;
        }
        if(!(new_fill_color == cur_text_state.fill_color))
        {
            cur_text_state.fill_color = new_fill_color;
            set_line_state(new_line_state, NLS_NEWSTATE);
        }
    }

    // stroke color
    if((!(param.fallback)) && (all_changed || stroke_color_changed))
    {
        // * PDF Spec. Table 106 –  Text rendering modes
        static const char STROKE[8] = { false, true, true, false, false, true, true, false };
        
        int idx = state->getRender();
        assert((idx >= 0) && (idx < 8));
        Color new_stroke_color;
        // stroke
        if(STROKE[idx])
        {
            new_stroke_color.transparent = false;
            state->getStrokeRGB(&new_stroke_color.rgb);
        }
        else
        {
            new_stroke_color.transparent = true;
        }
        if(!(new_stroke_color == cur_text_state.stroke_color))
        {
            cur_text_state.stroke_color = new_stroke_color;
            set_line_state(new_line_state, NLS_NEWSTATE);
        }
    }

    reset_state_change();
}

void HTMLRenderer::prepare_text_line(GfxState * state)
{
    if(!(html_text_page.get_cur_line()))
        new_line_state = NLS_NEWCLIP;

    if(new_line_state >= NLS_NEWCLIP)
    {
        html_text_page.clip(cur_clip_state);
    }
    
    if(new_line_state >= NLS_NEWLINE)
    {
        // update position such that they will be recorded by text_line_buf
        double rise_x, rise_y;
        state->textTransformDelta(0, state->getRise(), &rise_x, &rise_y);
        state->transform(state->getCurX() + rise_x, state->getCurY() + rise_y, &cur_line_state.x, &cur_line_state.y);

        if (param.correct_text_visibility)
            cur_line_state.first_char_index = get_char_count();

        html_text_page.open_new_line(cur_line_state);

        cur_text_state.vertical_align = 0;

        //resync position
        draw_ty = cur_ty;
        draw_tx = cur_tx;
    }
    else
    {
        // align horizontal position
        // try to merge with the last line if possible
        double target = (cur_tx - draw_tx) * draw_text_scale;
        if(!equal(target, 0))
        {
            html_text_page.get_cur_line()->append_offset(target);
            draw_tx += target / draw_text_scale;
        }
    }

    if(new_line_state != NLS_NONE)
    {
        html_text_page.get_cur_line()->append_state(cur_text_state);
    }
}

} //namespace pdf2htmlEX
