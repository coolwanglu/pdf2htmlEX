/*
 * DrawingTracer.h
 *
 *  Created on: 2014-6-15
 *      Author: duanyao
 */

#ifndef DRAWINGTRACER_H__
#define DRAWINGTRACER_H__

#include <functional>

#include <GfxState.h>
#include <cairo.h>

#include "Param.h"

namespace pdf2htmlEX
{

class DrawingTracer
{
public:
    /*
     * The callback to receive drawn event.
     * bbox in device space.
     */
    // a non-char graphics is drawn
    std::function<void(double * bbox)> on_non_char_drawn;
    // a char is drawn in the clip area
    std::function<void(double * bbox)> on_char_drawn;
    // a char is drawn out of/partially in the clip area
    std::function<void(double * bbox, bool patially)> on_char_clipped;

    DrawingTracer(const Param & param);
    virtual ~DrawingTracer();
    void reset(GfxState * state);

    /*
     * A character is drawing
     * x, y: glyph-drawing position, in PDF text object space.
     * ax, ay: glyph advance, in glyph space.
     */
    void draw_char(GfxState * state, double x, double y, double ax, double ay);
    /*
     * An image is drawing
     */
    void draw_image(GfxState * state);
    void update_ctm(GfxState * state, double m11, double m12, double m21, double m22, double m31, double m32);
    void clip(GfxState * state, bool even_odd = false);
    void clip_to_stroke_path(GfxState * state);
    void fill(GfxState * state, bool even_odd = false);
    void stroke(GfxState * state);
    void save();
    void restore();

private:
    void finish();
    // Following methods operate in user space (just before CTM is applied)
    void do_path(GfxState * state, GfxPath * path);
    void draw_non_char_bbox(GfxState * state, double * bbox);
    void draw_char_bbox(GfxState * state, double * bbox);
    void transform_bbox_by_ctm(double * bbox);

    const Param & param;
    cairo_t * cairo;
};

} /* namespace pdf2htmlEX */
#endif /* DRAWINGTRACER_H__ */
