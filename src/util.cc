/*
 * Misc functions
 *
 *
 * by WangLu
 * 2012.08.10
 */

#include <errno.h>
#include <cctype>

#include <GfxState.h>
#include <GfxFont.h>
#include <CharTypes.h>
#include <GlobalParams.h>
#include <Object.h>

// for mkdir
#include <sys/stat.h>
#include <sys/types.h>

#include "util.h"

using std::cerr;
using std::endl;
using std::string;
using std::map;
using std::ostream;

namespace pdf2htmlEX {

const double id_matrix[6] = {1.0, 0.0, 0.0, -1.0, 0.0, 0.0};

const map<string, string> BASE_14_FONT_CSS_FONT_MAP({
   { "Courier", "Courier,monospace" },
   { "Helvetica", "Helvetica,Arial,\"Nimbus Sans L\",sans-serif" },
   { "Times", "Times,\"Time New Roman\",\"Nimbus Roman No9 L\",serif" },
   { "Symbol", "Symbol,\"Standard Symbols L\"" },
   { "ZapfDingbats", "ZapfDingbats,\"Dingbats\"" },
});

const map<string, string> GB_ENCODED_FONT_NAME_MAP({
    {"\xCB\xCE\xCC\xE5", "SimSun"},
    {"\xBA\xDA\xCC\xE5", "SimHei"},
    {"\xBF\xAC\xCC\xE5_GB2312", "SimKai"},
    {"\xB7\xC2\xCB\xCE_GB2312", "SimFang"},
    {"\xC1\xA5\xCA\xE9", "SimLi"},
});

const std::map<std::pair<std::string, bool>, std::pair<std::string, std::string> > EMBED_STRING_MAP({
        {{".css", 0}, {"<link rel=\"stylesheet\" type=\"text/css\" href=\"", "\"/>"}},
        {{".css", 1}, {"<style type=\"text/css\">", "</style>"}},
        {{".js", 0}, {"<script type=\"text/javascript\" src=\"", "\"></script>"}},
        {{".js", 1}, {"<script type=\"text/javascript\">", "</script>"}}
});

void _tm_transform(const double * tm, double & x, double & y, bool is_delta)
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

void _tm_multiply(double * tm_left, const double * tm_right)
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

bool isLegalUnicode(Unicode u)
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

Unicode map_to_private(CharCode code)
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

Unicode unicode_from_font (CharCode code, GfxFont * font)
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

Unicode check_unicode(Unicode * u, int len, CharCode code, GfxFont * font)
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

/*
 * Copied from UTF.h / UTF8.h in poppler
 */
static int mapUTF8(Unicode u, char *buf, int bufSize) {
  if        (u <= 0x0000007f) {
    if (bufSize < 1) {
      return 0;
    }
    buf[0] = (char)u;
    return 1;
  } else if (u <= 0x000007ff) {
    if (bufSize < 2) {
      return 0;
    }
    buf[0] = (char)(0xc0 + (u >> 6));
    buf[1] = (char)(0x80 + (u & 0x3f));
    return 2;
  } else if (u <= 0x0000ffff) {
    if (bufSize < 3) {
      return 0;
    }
    buf[0] = (char)(0xe0 + (u >> 12));
    buf[1] = (char)(0x80 + ((u >> 6) & 0x3f));
    buf[2] = (char)(0x80 + (u & 0x3f));
    return 3;
  } else if (u <= 0x0010ffff) {
    if (bufSize < 4) {
      return 0;
    }
    buf[0] = (char)(0xf0 + (u >> 18));
    buf[1] = (char)(0x80 + ((u >> 12) & 0x3f));
    buf[2] = (char)(0x80 + ((u >> 6) & 0x3f));
    buf[3] = (char)(0x80 + (u & 0x3f));
    return 4;
  } else {
    return 0;
  }
}

void outputUnicodes(ostream & out, const Unicode * u, int uLen)
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

const char * base64stream::base64_encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void create_directories(string path)
{
    if(path.empty()) return;

    size_t idx = path.rfind('/');
    if(idx != string::npos)
    {
        create_directories(path.substr(0, idx));
    }
            
    int r = mkdir(path.c_str(), S_IRWXU);
    if(r != 0)
    {
        if(errno == EEXIST)
        {
            struct stat stat_buf;
            if((stat(path.c_str(), &stat_buf) == 0) && S_ISDIR(stat_buf.st_mode))
                return;
        }

        throw string("Cannot create directory: ") + path;
    }
}

bool is_truetype_suffix(const string & suffix)
{
    return (suffix == ".ttf") || (suffix == ".ttc") || (suffix == ".otf");
}

string get_filename (const string & path)
{
    size_t idx = path.rfind('/');
    if(idx == string::npos) 
        return path;
    else if (idx == path.size() - 1)
        return "";
    return path.substr(idx + 1);
}

string get_suffix(const string & path)
{
    string fn = get_filename(path);
    size_t idx = fn.rfind('.');
    if(idx == string::npos)
        return "";
    else
    {
        string s = fn.substr(idx);
        for(auto iter = s.begin(); iter != s.end(); ++iter)
            *iter = tolower(*iter);
        return s;
    }
}

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
