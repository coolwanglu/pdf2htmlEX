/*
 * Encodings used in HTML
 *
 * by WangLu
 * 2013.02.15
 */

#include <cstring>

#include "encoding.h"
#include "const.h" // for nullptr

namespace pdf2htmlEX {

using std::ostream;
using std::string;

/*
 * Copied from UTF.h / UTF8.h in poppler
 */
static int mapUTF8(Unicode u, char *buf, int bufSize) 
{
    if (u <= 0x0000007f) {
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

void writeUnicodes(ostream & out, const Unicode * u, int uLen)
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

/*
static void writeHEX(ostream & out, char c)
{
    static const char * hexchars = "0123456789abcdef";
    out << hexchars[(c>>4)&0xf] << hexchars[c&0xf];
}

void writeURL(ostream & out, const string & s)
{
    static char * dont_escape = nullptr;
    if(!dont_escape)
    {
        dont_escape = new char [256];
        memset(dont_escape, 0, 256 * sizeof(char));
        / *
         * http://tools.ietf.org/html/rfc3986#section-2
         *
         * Also includes '%', in case that the original url has been escaped
         * /
        const char * no_escape_chars = ":/?#[]@!$&'()*+,;="
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "-._~"
            "%";
        while(*no_escape_chars)
            dont_escape[(int)*(no_escape_chars++)] = 1;
    }

    for (auto iter = s.begin(); iter != s.end(); ++iter)
    {
        char c = *iter;
        if(dont_escape[(int)c])
            out << c;
        else
        {
            out << '%';
            writeHEX(out, c);
        }
    }
}
*/

void writeJSON(ostream & out, const string & s)
{
    for(auto c : s)
    {
        switch (c)
        {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\'': out << "\\\'"; break;
            case '/': out << "\\/"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
}

void writeAttribute(std::ostream & out, const std::string & s)
{
    for (auto c : s)
    {
        switch(c)
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
            case '`': // for IE: http://html5sec.org/#59
                out << "&#96;";
                break;
            default:
                out << c;
        }
    }
}

} //namespace pdf2htmlEX
