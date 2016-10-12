/*
 * DrawingTracer.cc
 *
 *  Created on: 2014-6-15
 *      Author: duanyao
 */

#include "GfxFont.h"

#include "util/math.h"
#include "DrawingTracer.h"

#if !ENABLE_SVG
#error "ENABLE_SVG must be enabled"
#endif

//#define DEBUG

namespace pdf2htmlEX
{

DrawingTracer::DrawingTracer(const Param & param): param(param), cairo(nullptr)
{
}

DrawingTracer::~DrawingTracer()
{
    finish();
}

void DrawingTracer::reset(GfxState *state)
{
    finish();

    // pbox is defined in device space, which is affected by zooming;
    // We want to trace in page space which is stable, so invert pbox by ctm.
    double pbox[] { 0, 0, state->getPageWidth(), state->getPageHeight() };
    Matrix ctm, ictm;
    state->getCTM(&ctm);
    ctm.invertTo(&ictm);
    tm_transform_bbox(ictm.m, pbox);
    cairo_rectangle_t page_box { pbox[0], pbox[1], pbox[2] - pbox[0], pbox[3] - pbox[1] };
    cairo_surface_t * surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &page_box);
    cairo = cairo_create(surface);

    ctm_stack.clear();
    double *identity = new double[6];
    tm_init(identity);
    ctm_stack.push_back(identity);

#ifdef DEBUG
    printf("DrawingTracer::reset:page bbox:[%f,%f,%f,%f]\n",pbox[0], pbox[1], pbox[2], pbox[3]);
#endif
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
    if (!param.correct_text_visibility)
        return;

    double *tmp = new double[6];
    tmp[0] = m11;
    tmp[1] = m12;
    tmp[2] = m21;
    tmp[3] = m22;
    tmp[4] = m31;
    tmp[5] = m32;
    double *ctm = ctm_stack.back();
    tm_multiply(ctm, tmp);
#ifdef DEBUG
    printf("DrawingTracer::before update_ctm:ctm:[%f,%f,%f,%f,%f,%f] => [%f,%f,%f,%f,%f,%f]\n", m11, m12, m21, m22, m31, m32, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
#endif
}

void DrawingTracer::clip(GfxState * state, bool even_odd)
{
    if (!param.correct_text_visibility)
        return;
    do_path(state, state->getPath());
    cairo_set_fill_rule(cairo, even_odd? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    cairo_clip (cairo);

#ifdef DEBUG
    {
        double cbox[4];
        cairo_clip_extents(cairo, cbox, cbox + 1, cbox + 2, cbox + 3);
        printf("DrawingTracer::clip:extents:even_odd=%d,[%f,%f,%f,%f]\n", even_odd, cbox[0],cbox[1],cbox[2],cbox[3]);
    }
#endif
}

void DrawingTracer::clip_to_stroke_path(GfxState * state)
{
    if (!param.correct_text_visibility)
        return;

    printf("TODO:clip_to_stroke_path\n");
    // TODO cairo_stroke_to_path() ?
}

void DrawingTracer::save()
{
    if (!param.correct_text_visibility)
        return;
    cairo_save(cairo);
    double *e = new double[6];
    memcpy(e, ctm_stack.back(), sizeof(double) * 6);
    ctm_stack.push_back(e);

#ifdef DEBUG
    printf("DrawingTracer::saved: [%f,%f,%f,%f,%f,%f]\n", e[0], e[1], e[2], e[3], e[4], e[5]);
#endif
}
void DrawingTracer::restore()
{
    if (!param.correct_text_visibility)
        return;
    cairo_restore(cairo);
    ctm_stack.pop_back();

#ifdef DEBUG
    double *ctm = ctm_stack.back();
    printf("DrawingTracer::restored: [%f,%f,%f,%f,%f,%f]\n", ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
#endif
}

void DrawingTracer::do_path(GfxState * state, GfxPath * path)
{
    //copy from CairoOutputDev::doPath
    GfxSubpath *subpath;
    int i, j;
    double x, y;
    cairo_new_path(cairo);
#ifdef DEBUG
    printf("DrawingTracer::do_path:new_path (%d subpaths)\n", path->getNumSubpaths());
#endif

    for (i = 0; i < path->getNumSubpaths(); ++i) {
        subpath = path->getSubpath(i);
        if (subpath->getNumPoints() > 0) {
            x = subpath->getX(0);
            y = subpath->getY(0);
            xform_pt(x, y);
            cairo_move_to(cairo, x, y);
            j = 1;
            while (j < subpath->getNumPoints()) {
                if (subpath->getCurve(j)) {
                    x = subpath->getX(j+2);
                    y = subpath->getY(j+2);
                    double x1 = subpath->getX(j);
                    double y1 = subpath->getY(j);
                    double x2 = subpath->getX(j+1);
                    double y2 = subpath->getY(j+1);
                    xform_pt(x, y);
                    xform_pt(x1, y1);
                    xform_pt(x2, y2);
                    cairo_curve_to(cairo,
                        x1, y1,
                        x2, y2,
                        x, y);
                    j += 3;
                } else {
                    x = subpath->getX(j);
                    y = subpath->getY(j);
                    xform_pt(x, y);
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
    if (!param.correct_text_visibility)
        return;


    // Transform the line width by the ctm. This isn't 100% - we should really do this path segment by path segment,
    // this is a reasonable approximation providing the CTM has uniform scaling X/Y
    double lwx, lwy;
    lwx = lwy = sqrt(0.5);
    tm_transform(ctm_stack.back(), lwx, lwy, true);
    double lineWidthScale = sqrt(lwx * lwx + lwy * lwy);
#ifdef DEBUG
    printf("DrawingTracer::stroke. line width = %f*%f, line cap = %d\n", lineWidthScale, state->getLineWidth(), state->getLineCap());
#endif
    cairo_set_line_width(cairo, lineWidthScale * state->getLineWidth());

        // Line cap is important - some PDF line widths are very large
    switch (state->getLineCap()) {
    case 0:
        cairo_set_line_cap (cairo, CAIRO_LINE_CAP_BUTT);
        break;
    case 1:
        cairo_set_line_cap (cairo, CAIRO_LINE_CAP_ROUND);
        break;
    case 2:
        cairo_set_line_cap (cairo, CAIRO_LINE_CAP_SQUARE);
        break;
    }

    GfxPath * path = state->getPath();
    for (int i = 0; i < path->getNumSubpaths(); ++i) {
        GfxSubpath * subpath = path->getSubpath(i);
        if (subpath->getNumPoints() <= 0)
            continue;
        double x = subpath->getX(0);
        double y = subpath->getY(0);
        xform_pt(x, y);
        //p: loop cursor; j: next point index
        int p =1;
        int n = subpath->getNumPoints();
        while (p < n) {
            cairo_new_path(cairo);
#ifdef DEBUG
printf("move_to: [%f,%f]\n", x, y);
#endif
            cairo_move_to(cairo, x, y);
            if (subpath->getCurve(p)) {
                x = subpath->getX(p+2);
                y = subpath->getY(p+2);
                double x1 = subpath->getX(p);
                double y1 = subpath->getY(p);
                double x2 = subpath->getX(p+1);
                double y2 = subpath->getY(p+1);
                xform_pt(x, y);
                xform_pt(x1, y1);
                xform_pt(x2, y2);
#ifdef DEBUG
printf("curve_to: [%f,%f], [%f,%f], [%f,%f]\n", x1, y1, x2, y2, x, y);
#endif
                cairo_curve_to(cairo,
                    x1, y1,
                    x2, y2,
                    x, y);
                p += 3;
            } else {
                x = subpath->getX(p);
                y = subpath->getY(p);
                xform_pt(x, y);
#ifdef DEBUG
printf("line_to: [%f,%f]\n", x, y);
#endif
                cairo_line_to(cairo, x, y);
                ++p;
            }

            double sbox[4];
            cairo_stroke_extents(cairo, sbox, sbox + 1, sbox + 2, sbox + 3);
#ifdef DEBUG
            printf("DrawingTracer::stroke:new box:[%f,%f,%f,%f]\n", sbox[0], sbox[1], sbox[2], sbox[3]);
#endif
            if (sbox[0] != sbox[2] && sbox[1] != sbox[3])
                draw_non_char_bbox(state, sbox, 2);
        }
    }
}

void DrawingTracer::fill(GfxState * state, bool even_odd)
{
    if (!param.correct_text_visibility)
        return;

    do_path(state, state->getPath());
    //cairo_fill_extents don't take fill rule into account.
    //cairo_set_fill_rule (cairo, even_odd? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    double fbox[4];
    cairo_fill_extents(cairo, fbox, fbox + 1, fbox + 2, fbox + 3);

#ifdef DEBUG
    printf("DrawingTracer::fill:[%f,%f,%f,%f]\n", fbox[0],fbox[1],fbox[2],fbox[3]);
#endif

    draw_non_char_bbox(state, fbox, 1);
}

void DrawingTracer::draw_non_char_bbox(GfxState * state, double * bbox, int what)
{
// what == 0 => just do bbox text
// what == 1 => stroke test
// what == 2 => fill test
    double cbox[4];
    cairo_clip_extents(cairo, cbox, cbox + 1, cbox + 2, cbox + 3);
    if(bbox_intersect(cbox, bbox))
    {
#ifdef DEBUG
        printf("DrawingTracer::draw_non_char_bbox:what=%d,[%f,%f,%f,%f]\n", what, bbox[0],bbox[1],bbox[2],bbox[3]);
#endif
        if (on_non_char_drawn)
            on_non_char_drawn(cairo, bbox, what);
    }
}

void DrawingTracer::draw_char_bbox(GfxState * state, double * bbox, int inTransparencyGroup)
{
    if (inTransparencyGroup || state->getFillOpacity() < 1.0 || state->getStrokeOpacity() < 1.0) {
        on_char_clipped(cairo, bbox, 0);
        return;
    }
    if (!param.correct_text_visibility) {
        double bbox[4] = { 0, 0, 0, 0 }; // bbox not relevant if not correcting text visibility
        on_char_drawn(cairo, bbox);
        return;
    }

    double cbox[4];
    cairo_clip_extents(cairo, cbox, cbox + 1, cbox + 2, cbox + 3);
#ifdef DEBUG
    printf("DrawingTracer::draw_char_bbox::char bbox[%f,%f,%f,%f],clip extents:[%f,%f,%f,%f]\n", bbox[0], bbox[1], bbox[2], bbox[3], cbox[0],cbox[1],cbox[2],cbox[3]);
#endif

    if (bbox_intersect(bbox, cbox)) {
#ifdef DEBUG
        printf("char intersects clip\n");
#endif
        int pts_visible = 0;
        // See which points are inside the current clip
        if (cairo_in_clip(cairo, bbox[0], bbox[1]))
            pts_visible |= 1;
        if (cairo_in_clip(cairo, bbox[2], bbox[1]))
            pts_visible |= 2;
        if (cairo_in_clip(cairo, bbox[2], bbox[3]))
            pts_visible |= 4;
        if (cairo_in_clip(cairo, bbox[0], bbox[3]))
            pts_visible |= 8;

        if (pts_visible == (1|2|4|8)) {
#ifdef DEBUG
            printf("char inside clip\n");
#endif
            on_char_drawn(cairo, bbox);
        } else {
#ifdef DEBUG
            printf("char partial clip (%x)\n", pts_visible);
#endif
            on_char_clipped(cairo, bbox, pts_visible);
        }
    } else {
#ifdef DEBUG
        printf("char outside clip\n");
#endif
        on_char_clipped(cairo, bbox, 0);
    }
}

void DrawingTracer::draw_image(GfxState *state)
{
    if (!param.correct_text_visibility)
        return;

    double x1, y1, x2, y2, x3, y3, x4, y4;
    x1 = x4 = y3 = y4 = 0;
    x2 = y2 = x3 = y1 = 1;

    xform_pt(x1, y1);
    xform_pt(x2, y2);
    xform_pt(x3, y3);
    xform_pt(x4, y4);

    cairo_new_path(cairo);
    cairo_move_to(cairo, x1, y1);
    cairo_line_to(cairo, x2, y2);
    cairo_line_to(cairo, x3, y3);
    cairo_line_to(cairo, x4, y4);
    cairo_close_path (cairo);
#ifdef DEBUG
    printf("draw_image: [%f,%f], [%f,%f], [%f,%f], [%f,%f]\n", x1, y1, x2, y2, x3, y3, x4, y4);
#endif

    double bbox[4] {0, 0, 1, 1};
    tm_transform_bbox(ctm_stack.back(), bbox);

    draw_non_char_bbox(state, bbox, 1);
}

void DrawingTracer::draw_char(GfxState *state, double x, double y, double ax, double ay, int inTransparencyGroup)
{
    if (ax == 0) { // To handle some characters which have zero advance. Give it some width so the bbox tests still work
        ax = 0.1;
    }
    if (ay == 0) {
        ay = 0.1;
    }
        
//printf("x=%f,y=%f,ax=%f,ay=%f\n", x, y, ax, ay);
    Matrix tm, itm;
    memcpy(tm.m, state->getTextMat(), sizeof(tm.m));

//printf("tm = %f,%f,%f,%f,%f,%f\n", tm.m[0], tm.m[1], tm.m[2], tm.m[3], tm.m[4], tm.m[5]);
    double cx = state->getCurX(), cy = state->getCurY(), fs = state->getFontSize(),
        ry = state->getRise(), h = state->getHorizScaling();

//printf("cx=%f,cy=%f,fs=%f,ry=%f,h=%f\n", cx,cy,fs,ry,h);

    //cx and cy has been transformed by text matrix, we need to reverse them.
    tm.invertTo(&itm);
    double char_cx, char_cy;
    itm.transform(cx, cy, &char_cx, &char_cy);

//printf("char_cx = %f, char_cy = %f\n", char_cx, char_cy); 

    //TODO Vertical? Currently vertical/type3 chars are treated as non-chars.
    double char_m[6] {fs * h, 0, 0, fs, char_cx + x, char_cy + y + ry};

    double final_m[6];
    tm_multiply(final_m, tm.m, char_m);

//printf("final_m = %f,%f,%f,%f,%f,%f\n", final_m[0], final_m[1], final_m[2], final_m[3], final_m[4], final_m[5]);
    double final_after_ctm[6];
    tm_multiply(final_after_ctm, ctm_stack.back(), final_m);
//printf("final_after_ctm= %f,%f,%f,%f,%f,%f\n", final_after_ctm[0], final_after_ctm[1], final_after_ctm[2], final_after_ctm[3], final_after_ctm[4], final_after_ctm[5]);
    auto font = state->getFont();
    double bbox[4] {0, 0, ax, ay};
//    double desc = font->getDescent(), asc = font->getAscent();
    if (font->getWMode() == 0)
    {
//        bbox[1] += desc;
//        bbox[3] += asc;
    }
    else
    {//TODO Vertical?
    }

//printf("bbox before: [%f,%f,%f,%f]\n", bbox[0],bbox[1],bbox[2],bbox[3]);
//printf("bbox after: [%f,%f,%f,%f]\n", bbox[0],bbox[1],bbox[2],bbox[3]);
    tm_transform_bbox(final_after_ctm, bbox);
//printf("bbox after: [%f,%f,%f,%f]\n", bbox[0],bbox[1],bbox[2],bbox[3]);
    draw_char_bbox(state, bbox, inTransparencyGroup);
}

void DrawingTracer::xform_pt(double & x, double & y) {
    tm_transform(ctm_stack.back(), x, y);
}

} /* namespace pdf2htmlEX */
