/*
 * Draw.cc
 *
 * Handling path drawing
 *
 * by WangLu
 * 2012.10.01
 */

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>
#include <iostream>

#include "HTMLRenderer.h"
#include "util.h"
#include "namespace.h"

namespace pdf2htmlEX {

using std::swap;
using std::min;
using std::max;
using std::acos;
using std::asin;
using std::ostringstream;
using std::sqrt;
using std::vector;
using std::ostream;

static bool is_horizontal_line(GfxSubpath * path)
{
    return ((path->getNumPoints() == 2)
            && (!path->getCurve(1))
            && (_equal(path->getY(0), path->getY(1))));
}

static bool is_vertical_line(GfxSubpath * path)
{
    return ((path->getNumPoints() == 2)
            && (!path->getCurve(1))
            && (_equal(path->getX(0), path->getX(1))));
}

static bool is_rectangle(GfxSubpath * path)
{
    if (!(((path->getNumPoints() != 4) && (path->isClosed()))
          || ((path->getNumPoints() == 5) 
              && _equal(path->getX(0), path->getX(4))
              && _equal(path->getY(0), path->getY(4)))))
            return false;

    for(int i = 1; i < path->getNumPoints(); ++i)
        if(path->getCurve(i))
            return false;

    return (_equal(path->getY(0), path->getY(1))
            && _equal(path->getX(1), path->getX(2))
            && _equal(path->getY(2), path->getY(3))
            && _equal(path->getX(3), path->getX(0)))
        || (_equal(path->getX(0), path->getX(1))
            && _equal(path->getY(1), path->getY(2))
            && _equal(path->getX(2), path->getX(3))
            && _equal(path->getY(3), path->getY(0)));
}

static void get_shading_bbox(GfxState * state, GfxShading * shading,
        double & x1, double & y1, double & x2, double & y2)
{
    // from SplashOutputDev.cc in poppler
    if(shading->getHasBBox())
    {
        shading->getBBox(&x1, &y1, &x2, &y2);
    }
    else
    {
        state->getClipBBox(&x1, &y1, &x2, &y2);
        Matrix ctm, ictm;
        state->getCTM(&ctm);
        ctm.invertTo(&ictm);

        double x[4], y[4];
        ictm.transform(x1, y1, &x[0], &y[0]);
        ictm.transform(x2, y1, &x[1], &y[1]);
        ictm.transform(x1, y2, &x[2], &y[2]);
        ictm.transform(x2, y2, &x[3], &y[3]);

        x1 = x2 = x[0];
        y1 = y2 = y[0];

        for(int i = 1; i < 4; ++i)
        {
            x1 = min<double>(x1, x[i]);
            y1 = min<double>(y1, y[i]);
            x2 = max<double>(x2, x[i]);
            y2 = max<double>(y2, y[i]);
        }
    }
}

/*
 * Note that the coordinate system in HTML and PDF are different
 * This functions returns the angle of vector (dx,dy) in the PDF coordinate system, in rad
 */
static double get_angle(double dx, double dy)
{
    double r = _hypot(dx, dy);

    /*
     * acos always returns [0, pi]
     */
    double ang = acos(dx / r);
    /*
     * for angle below x-axis
     */
    if(dy < 0)
        ang = -ang;

    return ang;
}

class LinearGradient
{
public:
    LinearGradient(GfxAxialShading * shading,
            double x1, double y1, double x2, double y2);

    void dumpto (ostream & out);

    static void style_function (void * p, ostream & out)
    {
        static_cast<LinearGradient*>(p)->dumpto(out);
    }

    // TODO, add alpha

    class ColorStop
    {
    public:
        GfxRGB rgb;
        double pos; // [0,1]
    };

    vector<ColorStop> stops;
    double angle;
};

LinearGradient::LinearGradient (GfxAxialShading * shading,
        double x1, double y1, double x2, double y2)
{
    // coordinate for t = 0 and t = 1
    double t0x, t0y, t1x, t1y;
    shading->getCoords(&t0x, &t0y, &t1x, &t1y);

    angle = get_angle(t1x - t0x, t1y - t0y);

    // get the range of t in the box
    // from GfxState.cc in poppler
    double box_tmin, box_tmax;
    {
        double idx = t1x - t0x;
        double idy = t1y - t0y;
        double inv_len = 1.0 / (idx * idx + idy * idy);
        idx *= inv_len;
        idy *= inv_len;

        // t of (x1,y1)
        box_tmin = box_tmax = (x1 - t0x) * idx + (y1 - t0y) * idy;
        double tdx = (x2 - x1) * idx;
        if(tdx < 0)
            box_tmin += tdx;
        else
            box_tmax += tdx;

        double tdy = (y2 - y1) * idy;
        if(tdy < 0)
            box_tmin += tdy; 
        else
            box_tmax += tdy;
    }

    // get the domain of t in the box
    double domain_tmin = max<double>(box_tmin, shading->getDomain0());
    double domain_tmax = min<double>(box_tmax, shading->getDomain1());

    // TODO: better sampling
    // TODO: check background color
    {
        stops.clear();
        double tstep = (domain_tmax - domain_tmin) / 13.0;
        for(double t = domain_tmin; t <= domain_tmax; t += tstep)
        {
            GfxColor color;
            shading->getColor(t, &color);

            ColorStop stop;
            shading->getColorSpace()->getRGB(&color, &stop.rgb);
            stop.pos = (t - box_tmin) / (box_tmax - box_tmin);

            stops.push_back(stop);
        }
    }
}

void LinearGradient::dumpto (ostream & out)
{
    auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
    for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
    {
        out << "background-image:" << (*iter) << "linear-gradient(" << _round(angle) << "rad";
        for(auto iter2 = stops.begin(); iter2 != stops.end(); ++iter2)
        {
            out << "," << (iter2->rgb) << " " << _round((iter2->pos) * 100) << "%";
        }
        out << ");";
    }
}

GBool HTMLRenderer::axialShadedFill(GfxState *state, GfxAxialShading *shading, double tMin, double tMax)
{
    if(!(param->css_draw)) return gFalse;

    double x1, y1, x2, y2;
    get_shading_bbox(state, shading, x1, y1, x2, y2);

    LinearGradient lg(shading, x1, y1, x2, y2);

    // TODO: check background color
    css_draw_rectangle(x1, y1, x2-x1, y2-y1, state->getCTM(),
            nullptr, 0,
            nullptr, nullptr,
            LinearGradient::style_function, &lg);

    return gTrue;
}

//TODO track state
//TODO connection style
bool HTMLRenderer::css_do_path(GfxState *state, bool fill, bool test_only)
{
    if(!(param->css_draw)) return false;

    GfxPath * path = state->getPath();
    /* 
     * capacity check
     */
    for(int i = 0; i < path->getNumSubpaths(); ++i)
    {
        GfxSubpath * subpath = path->getSubpath(i);
        if(!(is_horizontal_line(subpath)
             || is_vertical_line(subpath)
             || is_rectangle(subpath)))
            return false;
    }

    if(test_only) 
        return true;

    for(int i = 0; i < path->getNumSubpaths(); ++i)
    {
        GfxSubpath * subpath = path->getSubpath(i);

        if(is_horizontal_line(subpath))
        {
            double x1 = subpath->getX(0);
            double x2 = subpath->getX(1);
            double y = subpath->getY(0);
            if(x1 > x2) swap(x1, x2);

            GfxRGB stroke_color;
            state->getStrokeRGB(&stroke_color);

            double lw = state->getLineWidth();

            css_draw_rectangle(x1, y - lw/2, x2-x1, lw, state->getCTM(),
                    nullptr, 0,
                    nullptr, &stroke_color);
        }
        else if(is_vertical_line(subpath))
        {
            double x = subpath->getX(0);
            double y1 = subpath->getY(0);
            double y2 = subpath->getY(1);
            if(y1 > y2) swap(y1, y2);

            GfxRGB stroke_color;
            state->getStrokeRGB(&stroke_color);

            double lw = state->getLineWidth();

            css_draw_rectangle(x-lw/2, y1, lw, y2-y1, state->getCTM(),
                    nullptr, 0,
                    nullptr, &stroke_color);
        }
        else if(is_rectangle(subpath))
        {
            double x1 = subpath->getX(0);
            double x2 = subpath->getX(2);
            double y1 = subpath->getY(0);
            double y2 = subpath->getY(2);

            if(x1 > x2) swap(x1, x2);
            if(y1 > y2) swap(y1, y2);

            double x,y,w,h,lw[2];
            css_fix_rectangle_border_width(x1, y1, x2, y2, (fill ? 0.0 : state->getLineWidth()),
                    x,y,w,h,lw[0],lw[1]);

            GfxRGB stroke_color;
            if(!fill) state->getStrokeRGB(&stroke_color);

            GfxRGB fill_color;
            if(fill) state->getFillRGB(&fill_color);

            int lw_count = 2;

            GfxRGB * ps = fill ? nullptr : (&stroke_color);
            GfxRGB * pf = fill ? (&fill_color) : nullptr;

            if(_equal(h, 0) || _equal(w, 0))
            {
                // orthogonal line

                // TODO: check length
                pf = ps;
                ps = nullptr;
                h += lw[0];
                w += lw[1];
            }

            css_draw_rectangle(x, y, w, h, state->getCTM(),
                    lw, lw_count,
                    ps, pf);
        }
        else
        {
            assert(false);
        }
    }
    return true;
}

void HTMLRenderer::css_draw_rectangle(double x, double y, double w, double h, const double * tm,
        double * line_width_array, int line_width_count,
        const GfxRGB * line_color, const GfxRGB * fill_color, 
        void (*style_function)(void *, ostream &), void * style_function_data)
{
    close_text_line();

    double new_tm[6];
    memcpy(new_tm, tm, sizeof(new_tm));

    _tm_transform(new_tm, x, y);

    double scale = 1.0;
    {
        static const double sqrt2 = sqrt(2.0);

        double i1 = (new_tm[0] + new_tm[2]) / sqrt2;
        double i2 = (new_tm[1] + new_tm[3]) / sqrt2;
        scale = _hypot(i1, i2);
        if(_is_positive(scale))
        {
            for(int i = 0; i < 4; ++i)
                new_tm[i] /= scale;
        }
        else
        {
            scale = 1.0;
        }
    }


	device.css_draw_rectangle( x, y, w, h, scale, install_transform_matrix(new_tm),
							   line_width_array, line_width_count,
							   line_color, fill_color, 
							   style_function, style_function_data);
}


} // namespace pdf2htmlEX
