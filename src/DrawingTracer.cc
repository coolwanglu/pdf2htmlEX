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

// Poppler won't inform us its initial CTM, and the initial CTM is affected by zoom level.
// OutputDev::clip() may be called before OutputDev::updateCTM(), so we can't rely on GfxState::getCTM(),
// and should trace ctm changes ourself (via cairo).
void DrawingTracer::update_ctm(GfxState *state, double m11, double m12, double m21, double m22, double m31, double m32)
{
    if (!param.process_covered_text)
        return;
    cairo_matrix_t matrix;
    matrix.xx = m11;
    matrix.yx = m12;
    matrix.xy = m21;
    matrix.yy = m22;
    matrix.x0 = m31;
    matrix.y0 = m32;
    cairo_transform(cairo, &matrix);
}

void DrawingTracer::clip(GfxState * state, bool even_odd)
{
    if (!param.process_covered_text)
        return;
    do_path(state, state->getPath());
    cairo_set_fill_rule(cairo, even_odd? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
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
    cairo_new_path(cairo);
    for (i = 0; i < path->getNumSubpaths(); ++i) {
        subpath = path->getSubpath(i);
        if (subpath->getNumPoints() > 0) {
            x = subpath->getX(0);
            y = subpath->getY(0);
            cairo_move_to(cairo, x, y);
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
                    cairo_line_to(cairo, x, y);
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
    double cbox[4];
    cairo_clip_extents(cairo, cbox, cbox + 1, cbox + 2, cbox + 3);
    if(bbox_intersect(cbox, bbox, bbox))
    {
        transform_bbox_by_ctm(bbox);
        if (on_non_char_drawn)
            on_non_char_drawn(bbox);
    }
}

void DrawingTracer::draw_char_bbox(GfxState * state, double * bbox)
{
    // Note: even if 4 corners of the char are all in or all out of the clip area,
    // it could still be partially clipped.
    // TODO better solution?
    int pt_in = 0;
    if (cairo_in_clip(cairo, bbox[0], bbox[1]))
        ++pt_in;
    if (cairo_in_clip(cairo, bbox[2], bbox[3]))
        ++pt_in;
    if (cairo_in_clip(cairo, bbox[2], bbox[1]))
        ++pt_in;
    if (cairo_in_clip(cairo, bbox[0], bbox[3]))
        ++pt_in;

    if (pt_in == 0)
    {
        if(on_char_clipped)
            on_char_clipped(bbox, false);
    }
    else
    {
        if (pt_in < 4)
        {
            double cbox[4];
            cairo_clip_extents(cairo, cbox, cbox + 1, cbox + 2, cbox + 3);
            bbox_intersect(cbox, bbox, bbox);
        }
        transform_bbox_by_ctm(bbox);
        if (pt_in < 4)
        {
            if(on_char_clipped)
                on_char_clipped(bbox, true);
        }
        else
        {
            if (on_char_drawn)
                on_char_drawn(bbox);
        }
    }
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
    memcpy(tm.m, state->getTextMat(), sizeof(tm.m));

    double cx = state->getCurX(), cy = state->getCurY(), fs = state->getFontSize(),
            ry = state->getRise(), h = state->getHorizScaling();

    //cx and cy has been transformed by text matrix, we need to reverse them.
    tm.invertTo(&itm);
    double char_cx, char_cy;
    itm.transform(cx, cy, &char_cx, &char_cy);

    //TODO Vertical? Currently vertical/type3 chars are treated as non-chars.
    double char_m[6] {fs * h, 0, 0, fs, char_cx + x, char_cy + y + ry};

    double final_m[6];
    tm_multiply(final_m, tm.m, char_m);

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
    tm_transform_bbox(final_m, bbox);
    draw_char_bbox(state, bbox);
}

void DrawingTracer::transform_bbox_by_ctm(double * bbox)
{
    cairo_matrix_t mat;
    cairo_get_matrix(cairo, &mat);
    double mat_a[6] {mat.xx, mat.yx, mat.xy, mat.yy, mat.x0, mat.y0};
    tm_transform_bbox(mat_a, bbox);
}

} /* namespace pdf2htmlEX */
