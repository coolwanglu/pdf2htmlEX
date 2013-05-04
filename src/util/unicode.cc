/*
 * Unicode manipulation functions
 *
 * by WangLu
 * 2012.11.29
 */

#include <iostream>

#include <GlobalParams.h>

#include "unicode.h"

namespace pdf2htmlEX {

using std::cerr;
using std::endl;
using std::ostream;

/* 
 * Test legal for HTML 
 * 
 * A legal unicode character should be accepted by browsers, and displayed correctly.
 * This function is not complete, just to be improved.
 */
bool isLegalUnicode(Unicode u)
{
    /*
     * These characters are interpreted as white-spaces in HTML
     * `word-spacing` may be applied on them
     * and the browser may not use the actualy glyphs in the font
     * So mark them as illegal
     *
     * The problem is that the correct value can not be copied out in this way
     */
    /*
    if((u == 9) || (u == 10) || (u == 13))
        return true;
        */

    if(u <= 31) 
        return false;

    /*
     * 160, or 0xa0 is NBSP, which is legal in HTML
     * But some browser will use the glyph for ' ' in the font, it there is one, instead of the glyphs for NBSP
     * Again, `word-spacing` is applied.
     * So mark it as illegal
     *
     * And the same problem as above, this character can no longer be copied out
     */
    if((u >= 127) && (u <= 160))
        return false;

    /*
     * 173, or 0xad, is the soft hyphen
     * which can be ignored by the browser in the middle of a line
     */
    if(u == 173)
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

} //namespace pdf2htmlEX
