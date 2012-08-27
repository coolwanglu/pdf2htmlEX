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
#include <iostream>

#include <GfxState.h>
#include <GfxFont.h>
#include <CharTypes.h>
#include <UTF8.h>
#include <GlobalParams.h>

#include "Consts.h"

using std::istream;
using std::ostream;
using std::noskipws;
using std::endl;
using std::flush;
using std::cerr;

// mute gcc warning of unused function
namespace
{
    template <class T>
    void dummy(){ auto _ = &mapUCS2; }
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

/*
 * http://en.wikipedia.org/wiki/HTML_decimal_character_rendering
 */
static inline bool isLegalUnicode(Unicode u)
{
    /*
    if((u == 9) || (u == 10) || (u == 13))
        return true;
        */

    if(u <= 31) 
        return false;

    if((u >= 127) && (u <= 159))
        return false;

    if((u >= 0xd800) && (u <= 0xdfff))
        return false;

    return true;
}

static inline Unicode map_to_private(CharCode code)
{
    Unicode private_mapping = (Unicode)(code + 0xE000);
    if(private_mapping > 0xF8FF)
    {
        private_mapping = (Unicode)((private_mapping - 0xF8FF) + 0xF0000);
        if(private_mapping > 0xFFFFD)
        {
            private_mapping = (Unicode)((private_mapping - 0xFFFFD) + 0x100000);
            if(private_mapping > 0x10FFFD)
            {
                cerr << "Warning: all private use unicode are used" << endl;
            }
        }
    }
    return private_mapping;
}

/*
 * Try to determine the Unicode value directly from the information in the font
 */
static inline Unicode unicode_from_font (CharCode code, GfxFont * font)
{
    if(!font->isCIDFont())
    {
        char * cname = dynamic_cast<Gfx8BitFont*>(font)->getCharName(code);
        // may be untranslated ligature
        if(cname)
        {
            Unicode ou = globalParams->mapNameToUnicode(cname);

            if(isLegalUnicode(ou))
                return ou;
        }
    }

    return map_to_private(code);
}

/*
 * We have to use a single Unicode value to reencode fonts
 * if we got multi-unicode values, it might be expanded ligature, try to restore it
 * if we cannot figure it out at the end, use a private mapping
 */
static inline Unicode check_unicode(Unicode * u, int len, CharCode code, GfxFont * font)
{
    if(len == 0)
        return map_to_private(code);

    if(len == 1)
    {
        if(isLegalUnicode(*u))
            return *u;
    }

    return unicode_from_font(code, font);
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
    long long id;
    bool use_tounicode;
};

// wrapper of the transform matrix double[6]
// Transform Matrix
class TM
{
public:
    TM() {}
    TM(const double * m) {memcpy(_, m, sizeof(_));}
    bool operator < (const TM & m) const {
        // Note that we only care about the first 4 elements
        for(int i = 0; i < 4; ++i)
        {
            if(_[i] < m._[i] - EPS)
                return true;
            if(_[i] > m._[i] + EPS)
                return false;
        }
        return false;
    }
    bool operator == (const TM & m) const {
        return _tm_equal(_, m._, 4);
    }
    double _[6];
};

class base64stream
{
public:

    base64stream(istream & in) : in(&in) { }
    base64stream(istream && in) : in(&in) { }

    ostream & dumpto(ostream & out)
    {
        unsigned char buf[3];
        while(in->read((char*)buf, 3))
        {
            out << base64_encoding[(buf[0] & 0xfc)>>2]
                << base64_encoding[((buf[0] & 0x03)<<4) | ((buf[1] & 0xf0)>>4)]
                << base64_encoding[((buf[1] & 0x0f)<<2) | ((buf[2] & 0xc0)>>6)]
                << base64_encoding[(buf[2] & 0x3f)];
        } 
        auto cnt = in->gcount();
        if(cnt > 0)
        {
            for(int i = cnt; i < 3; ++i)
                buf[i] = 0;

            out << base64_encoding[(buf[0] & 0xfc)>>2]
                << base64_encoding[((buf[0] & 0x03)<<4) | ((buf[1] & 0xf0)>>4)];

            if(cnt > 1)
            {
                out << base64_encoding[(buf[1] & 0x0f)<<2];
            }
            else
            {
                out <<  '=';
            }
            out << '=';
        }

        return out;
    }

private:
    static constexpr const char * base64_encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    istream * in;
};

static inline ostream & operator << (ostream & out, base64stream & bf) { return bf.dumpto(out); }
static inline ostream & operator << (ostream & out, base64stream && bf) { return bf.dumpto(out); }

#endif //UTIL_H__
