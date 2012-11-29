/*
 * Misc functions
 *
 *
 * by WangLu
 * 2012.08.10
 */

#include <map>

#include "misc.h"

using std::cerr;
using std::endl;
using std::string;
using std::map;
using std::ostream;

namespace pdf2htmlEX {

void css_fix_rectangle_border_width(double x1, double y1, 
        double x2, double y2, 
        double border_width, 
        double & x, double & y, double & w, double & h,
        double & border_top_bottom_width, 
        double & border_left_right_width)
{
    w = x2 - x1;
    if(w > border_width)
    {
        w -= border_width;
        border_left_right_width = border_width;
    }
    else
    {
        border_left_right_width = border_width + w/2;
        w = 0;
    }
    x = x1 - border_width / 2;

    h = y2 - y1;
    if(h > border_width)
    {
        h -= border_width;
        border_top_bottom_width = border_width;
    }
    else
    {
        border_top_bottom_width = border_width + h/2;
        h = 0;
    }
    y = y1 - border_width / 2;
}

ostream & operator << (ostream & out, const GfxRGB & rgb)
{
    auto flags= out.flags();
    out << std::dec << "rgb(" 
        << (int)colToByte(rgb.r) << "," 
        << (int)colToByte(rgb.g) << "," 
        << (int)colToByte(rgb.b) << ")";
    out.flags(flags);
    return out;
}

} // namespace pdf2htmlEX
