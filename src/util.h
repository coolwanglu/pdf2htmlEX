/*
 * Misc functions
 *
 *
 * by WangLu
 * 2012.08.10
 */


#ifndef UTIL_H__
#define UTIL_H__

#include <algorithm>
#include <ostream>

#include <UTF8.h>

#include "Consts.h"

// mute gcc
namespace
{
    template <class T>
    void dummy1(){ auto _ = &mapUCS2; }
}

static inline bool _equal(double x, double y) { return std::abs(x-y) < EPS; }
static inline bool _is_positive(double x) { return x > EPS; }
static inline bool _tm_equal(const double * tm1, const double * tm2, int size = 6)
{
    for(int i = 0; i < size; ++i)
        if(!_equal(tm1[i], tm2[i]))
            return false;
    return true;
}

static inline void outputUnicodes(std::ostream & out, const Unicode * u, int uLen)
{
    for(int i = 0; i < uLen; ++i)
    {
        switch(u[i])
        {
            case '&':
                out << "&amp;";
                break;
            case '\"':
                out << "&quot;";
                break;
            case '\'':
                out << "&apos;";
                break;
            case '<':
                out << "&lt;";
                break;
            case '>':
                out << "&gt;";
                break;
            default:
                {
                    char buf[4];
                    auto n = mapUTF8(u[i], buf, 4);
                    out.write(buf, n);
                }
        }
    }
}

static inline bool operator < (const GfxRGB & rgb1, const GfxRGB & rgb2)
{
    if(rgb1.r < rgb2.r) return true;
    if(rgb1.r > rgb2.r) return false;
    if(rgb1.g < rgb2.g) return true;
    if(rgb1.g > rgb2.g) return false;
    return (rgb1.b < rgb2.b);
}

static inline bool operator == (const GfxRGB & rgb1, const GfxRGB & rgb2)
{
    return ((rgb1.r == rgb2.r) && (rgb1.g == rgb2.g) && (rgb1.b == rgb1.b));
}


#endif //UTIL_H__
