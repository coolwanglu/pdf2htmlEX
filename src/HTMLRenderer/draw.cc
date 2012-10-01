/*
 * Draw.cc
 *
 * Handling path drawing
 *
 * by WangLu
 * 2012.10.01
 */

#include "HTMLRenderer.h"
#include "util.h"
#include "namespace.h"

namespace pdf2htmlEX {

using std::swap;

static bool is_horizontal_line(GfxSubpath * path)
{
    return ((path->getNumPoints() == 2)
            && (!path->getCurve(1))
            && (_equal(path->getY(0), path->getY(1))));
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

//TODO track state
//TODO connection style
void HTMLRenderer::css_draw(GfxState *state, bool fill)
{
    GfxPath * path = state->getPath();
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

            css_draw_rectangle(x1, y - lw/2, x2-x1, lw,
                    nullptr, 0,
                    nullptr, &stroke_color, state);
        }
        else if (is_rectangle(subpath))
        {
            close_text_line();
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

            css_draw_rectangle(x, y, w, h,
                    lw, lw_count,
                    ps, pf, state);
        }
    }
}

void HTMLRenderer::css_draw_rectangle(double x, double y, double w, double h,
        double * line_width_array, int line_width_count,
        const GfxRGB * line_color, const GfxRGB * fill_color,
        GfxState * state)
{
    close_text_line();

    html_fout << "<div class=\"Cd t" << install_transform_matrix(state->getCTM()) << "\" style=\"";

    if(line_color)
    {
        html_fout << "border-color:" << *line_color << ";";

        html_fout << "border-width:";
        for(int i = 0; i < line_width_count; ++i)
        {
            if(i > 0) html_fout << ' ';

            double lw = line_width_array[i];
            html_fout << _round(lw);
            if(lw > EPS) html_fout << "px";
        }
        html_fout << ";";
    }
    else
    {
        html_fout << "border:none;";
    }

    if(fill_color)
    {
        html_fout << "background-color:" << (*fill_color) << ";";
    }
    else
    {
        html_fout << "background-color:transparent;";
    }

    _transform(state->getCTM(), x, y);

    html_fout << "bottom:" << _round(y) << "px;"
        << "left:" << _round(x) << "px;"
        << "width:" << _round(w) << "px;"
        << "height:" << _round(h) << "px;";

    html_fout << "\"></div>";
}


} // namespace pdf2htmlEX
