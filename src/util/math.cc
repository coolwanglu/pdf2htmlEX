#include <cstring>
#include "math.h"

namespace pdf2htmlEX {

void tm_transform(const double * tm, double & x, double & y, bool is_delta)
{
    double xx = x, yy = y;
    x = tm[0] * xx + tm[2] * yy;
    y = tm[1] * xx + tm[3] * yy;
    if(!is_delta)
    {
        x += tm[4];
        y += tm[5];
    }
}

void tm_multiply(double * tm_left, const double * tm_right)
{
    double old[4];
    memcpy(old, tm_left, sizeof(old));

    tm_left[0] = old[0] * tm_right[0] + old[2] * tm_right[1];
    tm_left[1] = old[1] * tm_right[0] + old[3] * tm_right[1];
    tm_left[2] = old[0] * tm_right[2] + old[2] * tm_right[3];
    tm_left[3] = old[1] * tm_right[2] + old[3] * tm_right[3];
    tm_left[4] += old[0] * tm_right[4] + old[2] * tm_right[5];
    tm_left[5] += old[1] * tm_right[4] + old[3] * tm_right[5];
}

} //namespace pdf2htmlEX 

