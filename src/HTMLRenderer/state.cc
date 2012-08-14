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

void HTMLRenderer::check_state_change(GfxState * state)
{
    bool close_line = false;

    if(all_changed || text_pos_changed)
    {
        if(!(abs(cur_ty - draw_ty) * draw_scale < param->v_eps))
        {
            close_line = true;
            draw_ty = cur_ty;
            draw_tx = cur_tx;
        }
    }

    // TODO, we may use nested span if only color has been changed
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

    bool need_rescale_font = false;
    if(all_changed || font_changed)
    {
        long long new_fn_id = install_font(state->getFont());

        if(!(new_fn_id == cur_fn_id))
        {
            close_line = true;
            cur_fn_id = new_fn_id;
        }

        if(!_equal(cur_font_size, state->getFontSize()))
        {
            cur_font_size = state->getFontSize();
            need_rescale_font = true;
        }
    }  

    // TODO
    // Rise, HorizScale etc
    if(all_changed || text_mat_changed || ctm_changed)
    {
        double new_ctm[6];
        double * m1 = state->getCTM();
        double * m2 = state->getTextMat();
        new_ctm[0] = m1[0] * m2[0] + m1[2] * m2[1];
        new_ctm[1] = m1[1] * m2[0] + m1[3] * m2[1];
        new_ctm[2] = m1[0] * m2[2] + m1[2] * m2[3];
        new_ctm[3] = m1[1] * m2[2] + m1[3] * m2[3];
        new_ctm[4] = new_ctm[5] = 0;

        if(!_tm_equal(new_ctm, cur_ctm))
        {
            need_rescale_font = true;
            memcpy(cur_ctm, new_ctm, sizeof(cur_ctm));
        }
    }

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
            draw_font_size = new_draw_font_size;
            cur_fs_id = install_font_size(draw_font_size);
            close_line = true;
        }
        if(!(_tm_equal(new_draw_ctm, draw_ctm)))
        {
            memcpy(draw_ctm, new_draw_ctm, sizeof(draw_ctm));
            cur_tm_id = install_transform_matrix(draw_ctm);
            close_line = true;
        }
    }

    // TODO: track these 
    /*
    if(!(_equal(s1->getCharSpace(), s2->getCharSpace()) && _equal(s1->getWordSpace(), s2->getWordSpace())
         && _equal(s1->getHorizScaling(), s2->getHorizScaling())))
            return false;
            */

    reset_state_track();

    if(close_line)
        close_cur_line();
}
void HTMLRenderer::reset_state_track()
{
    all_changed = false;
    text_pos_changed = false;
    ctm_changed = false;
    text_mat_changed = false;
    font_changed = false;
    color_changed = false;
}
void HTMLRenderer::close_cur_line()
{
    if(line_opened)
    {
        html_fout << "</div>" << endl;
        line_opened = false;
    }
}

void HTMLRenderer::updateAll(GfxState * state) 
{ 
    all_changed = true; 
    updateTextPos(state);
}

void HTMLRenderer::updateFont(GfxState * state) 
{
    font_changed = true; 
}

void HTMLRenderer::updateTextMat(GfxState * state) 
{
    text_mat_changed = true; 
}

void HTMLRenderer::updateCTM(GfxState * state, double m11, double m12, double m21, double m22, double m31, double m32) 
{
    ctm_changed = true; 
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

void HTMLRenderer::updateFillColor(GfxState * state) 
{
    color_changed = true; 
}
