#include <cmath>

#include "Color.h"

#include "util/misc.h"

namespace pdf2htmlEX {

using std::ostream;

Color::Color()
{
    memset(this, 0, sizeof(Color));
}

Color::Color(double r, double g, double b, bool transparent)
    :transparent(transparent)
{
    rgb.r = (GfxColorComp)(r * gfxColorComp1);
    rgb.g = (GfxColorComp)(g * gfxColorComp1);
    rgb.b = (GfxColorComp)(b * gfxColorComp1);
}

Color::Color(const GfxRGB& rgb)
    :transparent(false), rgb(rgb) { }

ostream & operator << (ostream & out, const Color & color)
{
    if(color.transparent)
        out << "transparent";
    else
        out << color.rgb;
    return out;
}

void Color::get_gfx_color(GfxColor & gc) const
{
    gc.c[0] = rgb.r;
    gc.c[1] = rgb.g;
    gc.c[2] = rgb.b;
}

double Color::distance(const Color & other) const
{
    double dr = (double)rgb.r - other.rgb.r,
            dg = (double)rgb.g - other.rgb.g,
            db = (double)rgb.b - other.rgb.b;
    return sqrt((dr * dr + dg * dg + db * db) / (3.0 * gfxColorComp1 * gfxColorComp1));
}

} // namespace pdf2htmlEX
