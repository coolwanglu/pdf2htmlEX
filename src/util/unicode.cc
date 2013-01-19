/*
 * Unicode manipulation functions
 *
 * by WangLu
 * 2012.11.29
 */

#include <GlobalParams.h>

#include "unicode.h"

namespace pdf2htmlEX {

using std::cerr;
using std::endl;
using std::ostream;

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

} //namespace pdf2htmlEX
