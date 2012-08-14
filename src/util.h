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
#include <istream>
#include <ostream>
#include <iterator>

#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>

#include <UTF8.h>

#include "Consts.h"

using std::istream;
using std::ostream;
using std::istream_iterator;
using std::ostream_iterator;
using std::copy;

using boost::archive::iterators::base64_from_binary;
using boost::archive::iterators::transform_width;

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

// we may need more info of a font in the future
class FontInfo
{
public:
    long long fn_id;
};

// wrapper of the transform matrix double[6]
// Transform Matrix
class TM
{
public:
    TM() {}
    TM(const double * m) {memcpy(_, m, sizeof(_));}
    bool operator < (const TM & m) const {
        for(int i = 0; i < 6; ++i)
        {
            if(_[i] < m._[i] - EPS)
                return true;
            if(_[i] > m._[i] + EPS)
                return false;
        }
        return false;
    }
    bool operator == (const TM & m) const {
        return _tm_equal(_, m._);
    }
    double _[6];
};

static inline void copy_base64 (ostream & out, istream & in, size_t length)
{
    typedef base64_from_binary < transform_width < istream_iterator<char>, 6, 8 > > base64_iter;
    copy(base64_iter(istream_iterator<char>(in)), base64_iter(istream_iterator<char>()), ostream_iterator<char>(out));
    switch(length % 3)
    {
        case 1:
            out << '=';
            // fall through
        case 2:
            out << '=';
            // fall through
        case 0:
        default:
            break;
    }
}

static inline void copy_base64 (ostream & out, istream && in, size_t length)
{
    typedef base64_from_binary < transform_width < istream_iterator<char>, 6, 8 > > base64_iter;
    copy(base64_iter(istream_iterator<char>(in)), base64_iter(istream_iterator<char>()), ostream_iterator<char>(out));
    switch(length % 3)
    {
        case 1:
            out << '=';
            // fall through
        case 2:
            out << '=';
            // fall through
        case 0:
        default:
            break;
    }
}

#endif //UTIL_H__
