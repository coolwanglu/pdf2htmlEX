/*
 * Constants & Misc functions
 *
 *
 * by WangLu
 * 2012.08.10
 */


#ifndef UTIL_H__
#define UTIL_H__

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <map>

#include <UTF8.h>

#ifndef nullptr
#define nullptr (NULL)
#endif

namespace pdf2htmlEX {

static const double EPS = 1e-6;
extern const double id_matrix[6];

static const double DEFAULT_DPI = 72.0;

extern const std::map<std::string, std::string> BASE_14_FONT_CSS_FONT_MAP;
extern const std::map<std::string, std::string> GB_ENCODED_FONT_NAME_MAP;
// map to embed files into html
// key: (suffix, if_embed_content)
// value: (prefix string, suffix string)
extern const std::map<std::pair<std::string, bool>, std::pair<std::string, std::string> > EMBED_STRING_MAP;

// mute gcc warning of unused function
namespace
{
    template <class T>
    void _(){ 
        auto _1 = &mapUCS2; 
        auto _2 = &mapUTF8;
    }
}

static inline double _round(double x) { return (std::abs(x) > EPS) ? x : 0.0; }
static inline bool _equal(double x, double y) { return std::abs(x-y) < EPS; }
static inline bool _is_positive(double x) { return x > EPS; }
static inline bool _tm_equal(const double * tm1, const double * tm2, int size = 6)
{
    for(int i = 0; i < size; ++i)
        if(!_equal(tm1[i], tm2[i]))
            return false;
    return true;
}

static inline long long hash_ref(const Ref * id)
{
    return (((long long)(id->num)) << (sizeof(id->gen)*8)) | (id->gen);
}

/*
 * http://en.wikipedia.org/wiki/HTML_decimal_character_rendering
 */
bool isLegalUnicode(Unicode u);

Unicode map_to_private(CharCode code);

/*
 * Try to determine the Unicode value directly from the information in the font
 */
Unicode unicode_from_font (CharCode code, GfxFont * font);

/*
 * We have to use a single Unicode value to reencode fonts
 * if we got multi-unicode values, it might be expanded ligature, try to restore it
 * if we cannot figure it out at the end, use a private mapping
 */
Unicode check_unicode(Unicode * u, int len, CharCode code, GfxFont * font);

void outputUnicodes(std::ostream & out, const Unicode * u, int uLen);

class GfxRGB_hash 
{
public:
    size_t operator () (const GfxRGB & rgb) const
    {
        return (colToByte(rgb.r) << 16) | (colToByte(rgb.g) << 8) | (colToByte(rgb.b));
    }
};

class GfxRGB_equal
{ 
public:
    bool operator ()(const GfxRGB & rgb1, const GfxRGB & rgb2) const
    {
        return ((rgb1.r == rgb2.r) && (rgb1.g == rgb2.g) && (rgb1.b == rgb1.b));
    }
};

// we may need more info of a font in the future
class FontInfo
{
public:
    long long id;
    bool use_tounicode;
    double ascent, descent;
    bool has_space; // whether space is included in the font
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

    base64stream(std::istream & in) : in(&in) { }
    base64stream(std::istream && in) : in(&in) { }

    std::ostream & dumpto(std::ostream & out)
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
    std::istream * in;
    static const char * base64_encoding;
};

static inline std::ostream & operator << (std::ostream & out, base64stream & bf) { return bf.dumpto(out); }
static inline std::ostream & operator << (std::ostream & out, base64stream && bf) { return bf.dumpto(out); }

class string_formatter
{
public:
    class guarded_pointer
    {
    public:
        guarded_pointer(string_formatter * sf) : sf(sf) { ++(sf->buf_cnt); }
        ~guarded_pointer(void) { --(sf->buf_cnt); }
        operator char* () { return &(sf->buf.front()); }
    private:
        string_formatter * sf;
    };

    string_formatter() : buf_cnt(0) { buf.reserve(L_tmpnam); }
    /*
     * Important:
     * there is only one buffer, so new strings will replace old ones
     */
    guarded_pointer operator () (const char * format, ...) {
        assert((buf_cnt == 0) && "string_formatter: buffer is reused!");

        va_list vlist;
        va_start(vlist, format);
        int l = vsnprintf(&buf.front(), buf.capacity(), format, vlist);
        va_end(vlist);
        if(l >= (int)buf.capacity()) 
        {
            buf.reserve(std::max((long)(l+1), (long)buf.capacity() * 2));
            va_start(vlist, format);
            l = vsnprintf(&buf.front(), buf.capacity(), format, vlist);
            va_end(vlist);
        }
        assert(l >= 0); // we should fail when vsnprintf fail
        assert(l < (int)buf.capacity());
        return guarded_pointer(this);
    }
private:
    friend class guarded_pointer;
    std::vector<char> buf;
    int buf_cnt;
};

void create_directories(std::string path);

bool is_truetype_suffix(const std::string & suffix);

std::string get_filename(const std::string & path);
std::string get_suffix(const std::string & path);

} // namespace util
#endif //UTIL_H__
