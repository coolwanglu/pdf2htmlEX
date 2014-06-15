/*
 * DrawingTracer.cc
 *
 *  Created on: 2014-6-15
 *      Author: duanyao
 */

#include "GfxFont.h"

#include "util/math.h"
#include "DrawingTracer.h"

namespace pdf2htmlEX
{

DrawingTracer::DrawingTracer(const Param & param):param(param)
{
}

DrawingTracer::~DrawingTracer()
{
    finish();
}

void DrawingTracer::reset(GfxState *state)
{
    if (!param.process_covered_text)
        return;
    finish();
    cairo_rectangle_t page_box {0, 0, width:state->getPageWidth(), height:state->getPageHeight()};
    cairo_surface_t * surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &page_box);
    cairo = cairo_create(surface);
}

void DrawingTracer::finish()
{
    if (cairo)
    {
        cairo_destroy(cairo);
        cairo = nullptr;
    }
}

void DrawingTracer::set_ctm(GfxState *state)
{
    if (!param.process_covered_text)
        return;
    double * ctm = state->getCTM();
    cairo_matrix_t matrix;
    matrix.xx = ctm[0];
    matrix.yx = ctm[1];
    matrix.xy = ctm[2];
    matrix.yy = ctm[3];
    matrix.x0 = ctm[4];
    matrix.y0 = ctm[5];
    cairo_set_matrix (cairo, &matrix);
}

void DrawingTracer::clip(GfxState * state, bool even_odd)
{
    if (!param.process_covered_text)
        return;
    do_path (state, state->getPath());
    cairo_set_fill_rule (cairo, even_odd? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    cairo_clip (cairo);
}

void DrawingTracer::clip_to_stroke_path(GfxState * state)
{
    if (!param.process_covered_text)
        return;
    // TODO cairo_stroke_to_path() ?
}

void DrawingTracer::save()
{
    if (!param.process_covered_text)
        return;
    cairo_save(cairo);
}
void DrawingTracer::restore()
{
    if (!param.process_covered_text)
        return;
    cairo_restore(cairo);
}

void DrawingTracer::do_path(GfxState * state, GfxPath * path)
{
    //copy from CairoOutputDev::doPath
    GfxSubpath *subpath;
    int i, j;
    double x, y;
    cairo_new_path (cairo);
    for (i = 0; i < path->getNumSubpaths(); ++i) {
        subpath = path->getSubpath(i);
        if (subpath->getNumPoints() > 0) {
            x = subpath->getX(0);
            y = subpath->getY(0);
            cairo_move_to (cairo, x, y);
            j = 1;
            while (j < subpath->getNumPoints()) {
                if (subpath->getCurve(j)) {
                    x = subpath->getX(j+2);
                    y = subpath->getY(j+2);
                    cairo_curve_to(cairo,
                        subpath->getX(j), subpath->getY(j),
                        subpath->getX(j+1), subpath->getY(j+1),
                        x, y);
                    j += 3;
                } else {
                    x = subpath->getX(j);
                    y = subpath->getY(j);
                    cairo_line_to (cairo, x, y);
                    ++j;
                }
            }
            if (subpath->isClosed()) {
                cairo_close_path (cairo);
            }
        }
    }
}

void DrawingTracer::stroke(GfxState * state)
{
    if (!param.process_covered_text)
        return;
    // TODO
    // 1. if stroke extents is large, break the path into pieces and handle each of them;
    // 2. if the line width is small, could just ignore the path?
    do_path(state, state->getPath());
    cairo_set_line_width(cairo, state->getLineWidth());
    double sbox[4];
    cairo_stroke_extents(cairo, sbox, sbox + 1, sbox + 2, sbox + 3);
    draw_non_char_bbox(state, sbox);
}

void DrawingTracer::fill(GfxState * state, bool even_odd)
{
    if (!param.process_covered_text)
        return;
    do_path(state, state->getPath());
    //cairo_fill_extents don't take fill rule into account.
    //cairo_set_fill_rule (cairo, even_odd? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    double fbox[4];
    cairo_fill_extents(cairo, fbox, fbox + 1, fbox + 2, fbox + 3);
    draw_non_char_bbox(state, fbox);
}

void DrawingTracer::draw_non_char_bbox(GfxState * state, double * bbox)
{
    double cbox[4], result[4];
    cairo_clip_extents(cairo, cbox, cbox + 1, cbox + 2, cbox + 3);
    // TODO intersect
    tm_transform_bbox(state->getCTM(), bbox);
    if (on_non_char_drawn)
        on_non_char_drawn(bbox);
}

void DrawingTracer::draw_char_bbox(GfxState * state, double * bbox)
{
    double cbox[4], result[4];
    cairo_clip_extents(cairo, cbox, cbox + 1, cbox + 2, cbox + 3);
    // TODO intersect
    tm_transform_bbox(state->getCTM(), bbox);
    if (on_char_drawn)
        on_char_drawn(bbox);
}

void DrawingTracer::draw_image(GfxState *state)
{
    if (!param.process_covered_text)
        return;
    double bbox[4] {0, 0, 1, 1};
    draw_non_char_bbox(state, bbox);
}

void DrawingTracer::draw_char(GfxState *state, double x, double y, double ax, double ay)
{
    if (!param.process_covered_text)
        return;

    Matrix tm, itm;
    //memcpy(tm_ctm.m, this->cur_text_tm, sizeof(tm_ctm.m));
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
    tm_multiply(tfinal, tm.m, tchar);

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
    draw_char_bbox(state, bbox);
}

} /* namespace pdf2htmlEX */
