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
 * Many unicode codes have special meaning which will be 'interpreted' by the browser, those should be filtered since they are not interpreted in PDF
 * This function is not complete, just to be improved.
 */
bool isLegalUnicode(Unicode u)
{
    const Unicode max_small_unicode = 1024;
    static bool valid_small_unicode[max_small_unicode];
    static bool valid_small_unicode_init = false;
    if(!valid_small_unicode_init)
    {
        valid_small_unicode_init = true;
        Unicode uu = 0;

        /*
         * 9, 10 and 13 are interpreted as white-spaces in HTML
         * `word-spacing` may be applied on them
         * and the browser may not use the actualy glyphs in the font
         * So mark them as illegal
         *
         * The problem is that the correct value can not be copied out in this way
         */
        while(uu <= 31)
            valid_small_unicode[uu++] = false;

        /*
         * 127-159 are not invalid
         * 160, or 0xa0 is NBSP, which is legal in HTML
         * But some browser will use the glyph for ' ' in the font, it there is one, instead of the glyphs for NBSP
         * Again, `word-spacing` is applied.
         * So mark it as illegal
         *
         * And the same problem as above, this character can no longer be copied out
         */
        while(uu < 127)
            valid_small_unicode[uu++] = true;
        while(uu <= 160)
            valid_small_unicode[uu++] = false;
        
        /*
         * 173, or 0xad, the soft hyphen
         * which can be ignored by the browser in the middle of a line
         */
        while(uu < 173)
            valid_small_unicode[uu++] = true;
        while(uu <= 173)
            valid_small_unicode[uu++] = false;


        while(uu < max_small_unicode)
            valid_small_unicode[uu++] = true;
    }

    if(u < max_small_unicode)
        return valid_small_unicode[u];

    /*
     * Reserved code for utf-16
     */
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
