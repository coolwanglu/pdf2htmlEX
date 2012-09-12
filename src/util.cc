/*
 * Misc functions
 *
 *
 * by WangLu
 * 2012.08.10
 */

#include <errno.h>

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

const double id_matrix[6] = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};

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
        return fn.substr(idx);
}

} // namespace pdf2htmlEX
