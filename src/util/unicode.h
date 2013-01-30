/*
 * Unicode manipulation functions
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef UNICODE_H__
#define UNICODE_H__

#include <iostream>

#include <GfxFont.h>
#include <CharTypes.h>

namespace pdf2htmlEX {

/*
 * Check if the unicode is valid for HTML
 * http://en.wikipedia.org/wiki/HTML_decimal_character_rendering
 */
bool isLegalUnicode(Unicode u);

Unicode map_to_private(CharCode code);

/* * Try to determine the Unicode value directly from the information in the font */
Unicode unicode_from_font (CharCode code, GfxFont * font);

/*
 * We have to use a single Unicode value to reencode fonts
 * if we got multi-unicode values, it might be expanded ligature, try to restore it
 * if we cannot figure it out at the end, use a private mapping
 */
Unicode check_unicode(Unicode * u, int len, CharCode code, GfxFont * font);

/*
 * Escape necessary characters, and map Unicode to UTF-8
 */
void outputUnicodes(std::ostream & out, const Unicode * u, int uLen);


} // namespace pdf2htmlEX

#endif //UNICODE_H__
