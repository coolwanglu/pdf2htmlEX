/*
 * state.cc
 *
 * track the current state
 *
 * by WangLu
 * 2012.08.14
 */


#include "HTMLRenderer.h"
#include "namespace.h"

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

void HTMLRenderer::updateFillColor(GfxState * state) 
{
    color_changed = true; 
}
void HTMLRenderer::check_state_change(GfxState * state)
{
    // DEPENDENCY WARNING
    // don't adjust the order of state checking 
    
    bool close_line = false;

    bool need_recheck_position = false;
    bool need_rescale_font = false;

    // rise
    if(all_changed || rise_changed)
    {
        double new_rise = state->getRise();
        if(!_equal(cur_rise, new_rise))
        {
            need_recheck_position = true;
            cur_rise = new_rise;
        }
    }

    // text position
    // we've been tracking the text position positively in update... function
    if(all_changed || text_pos_changed)
    {
        need_recheck_position = true;
    }

    // draw_tx, draw_ty
    // depends: rise & text position
    if(need_recheck_position)
    {
        // it's ok to use the old draw_scale
        // should draw_scale be updated, we'll close the line anyway
        if(!(abs((cur_ty + cur_rise) - draw_ty) * draw_scale < param->v_eps))
        {
            close_line = true;
        }
    }

    // font name & size
    if(all_changed || font_changed)
    {
        long long new_fn_id = install_font(state->getFont());

        if(!(new_fn_id == cur_fn_id))
        {
            close_line = true;
            cur_fn_id = new_fn_id;
        }

        double new_font_size = state->getFontSize();
        if(!_equal(cur_font_size, new_font_size))
        {
            need_rescale_font = true;
            cur_font_size = new_font_size;
        }
    }  

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

        if(!_tm_equal(new_ctm, cur_ctm))
        {
            need_rescale_font = true;
            memcpy(cur_ctm, new_ctm, sizeof(cur_ctm));
        }
    }

    // draw_ctm, draw_scale
    // depends: font size & ctm & text_ctm & hori scale
    if(need_rescale_font)
    {
        double new_draw_ctm[6];
        memcpy(new_draw_ctm, cur_ctm, sizeof(new_draw_ctm));

        draw_scale = sqrt(new_draw_ctm[2] * new_draw_ctm[2] + new_draw_ctm[3] * new_draw_ctm[3]);

        double new_draw_font_size = cur_font_size;
        if(_is_positive(draw_scale))
        {
            new_draw_font_size *= draw_scale;
            for(int i = 0; i < 4; ++i)
                new_draw_ctm[i] /= draw_scale;
        }
        else
        {
            draw_scale = 1.0;
        }

        if(!(_equal(new_draw_font_size, draw_font_size)))
        {
            close_line = true;
            draw_font_size = new_draw_font_size;
            cur_fs_id = install_font_size(draw_font_size);
        }
        if(!(_tm_equal(new_draw_ctm, draw_ctm)))
        {
            close_line = true;
            memcpy(draw_ctm, new_draw_ctm, sizeof(draw_ctm));
            cur_tm_id = install_transform_matrix(draw_ctm);
        }
    }

    // letter space
    // depends: draw_scale
    if(all_changed || letter_space_changed)
    {
        double new_letter_space = state->getCharSpace();
        if(!_equal(cur_letter_space, new_letter_space))
        {
            close_line = true;
            cur_letter_space = new_letter_space;
            cur_ls_id = install_letter_space(cur_letter_space * draw_scale);
        }
    }

    // word space
    // depends draw_scale
    if(all_changed || word_space_changed)
    {
        double new_word_space = state->getWordSpace();
        if(!_equal(cur_word_space, new_word_space))
        {
            close_line = true;
            cur_word_space = new_word_space;
            cur_ws_id = install_word_space(cur_word_space * draw_scale);
        }
    }

    // TODO, we may use nested span if only color is changed
   
    // color
    if(all_changed || color_changed)
    {
        GfxRGB new_color;
        state->getFillRGB(&new_color);
        if(!((new_color.r == cur_color.r) && (new_color.g == cur_color.g) && (new_color.b == cur_color.b)))
        {
            close_line = true;
            cur_color = new_color;
            cur_color_id = install_color(&new_color);
        }
    }

    reset_state_track();

    if(close_line)
        close_cur_line();
}
void HTMLRenderer::reset_state_track()
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

    color_changed = false;
}
void HTMLRenderer::close_cur_line()
{
    switch(line_status)
    {
        case LineStatus::SPAN:
            html_fout << "</span>";
            // fall through
        case LineStatus::DIV:
            html_fout << "</div>" << endl;
            line_status = LineStatus::CLOSED;
            break;

        case LineStatus::CLOSED:
        default:
            break;
    }

    draw_ty = cur_ty + cur_rise;
    draw_tx = cur_tx;
}

