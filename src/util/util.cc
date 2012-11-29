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
