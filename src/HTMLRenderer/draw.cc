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
void HTMLRenderer::stroke(GfxState *state)
{
    GfxPath * path = state->getPath();
    for(int i = 0; i < path->getNumSubpaths(); ++i)
    {
        GfxSubpath * subpath = path->getSubpath(i);

        if(is_horizontal_line(subpath))
        {
            close_text_line();
            double x1 = subpath->getX(0);
            double x2 = subpath->getX(1);
            double y = subpath->getY(0);
            if(x1 > x2) swap(x1, x2);

            _transform(state->getCTM(), x1, y);

            html_fout << "<div style=\"border:solid red;border-width:1px 0 0 0;position:absolute;bottom:" 
                << _round(y) << "px;left:" 
                << _round(x1) << "px;width:"
                << _round(x2-x1) << "px;height:0;\"></div>";
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

            double x,y,w,h,w1,w2;
            css_fix_rectangle_border_width(x1, y1, x2, y2, state->getLineWidth(), 
                    x,y,w,h,w1,w2);
            
            _transform(state->getCTM(), x, y);

            html_fout << "<div class=\"cr t" << install_transform_matrix(state->getCTM())
                << "\" style=\"border:solid red;border-width:"
                << _round(w1) << "px " 
                << _round(w2) << " px;left:" 
                << _round(x) << "px;bottom:" 
                << _round(y) << "px;width:"
                << _round(w) << "px;height:"
                << _round(h) << "px;\"></div>";
        }
    }
}

} // namespace pdf2htmlEX
