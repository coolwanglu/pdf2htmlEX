#include "Color.h"

#include "util/misc.h"

namespace pdf2htmlEX {

using std::ostream;

ostream & operator << (ostream & out, const Color & color)
{
    if(color.transparent)
        out << "transparent";
    else
        out << color.rgb;
    return out;
}

} // namespace pdf2htmlEX
