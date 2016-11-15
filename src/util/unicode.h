/*
 * Unicode manipulation functions
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef UNICODE_H__
#define UNICODE_H__

#include <GfxFont.h>
#include <CharTypes.h>

namespace pdf2htmlEX {

/**
 * Check whether a unicode character is illegal for the output HTML.
 * Unlike PDF readers, browsers has special treatments for such characters (normally treated as
 * zero-width space), regardless of metrics and glyphs provided by fonts. So these characters
 * should be mapped to unicode private area to "cheat" browsers, at the cost of losing actual
 * unicode values in the HTML.
 *
 * The following chart shows illegal characters  in HTML by webkit, mozilla, and pdf2htmlEX (p2h).
 * pdf2htmlEX's illegal character set is the union of webkit's and mozilla's, plus illegal unicode
 * characters. "[" and ")" surrounding ranges denote "inclusive" and "exclusive", respectively.
 *
 *         00(NUL)--09(\t)--0A(\n)--0D(\r)--20(SP)--7F(DEL)--9F(APC)--A0(NBSP)--AD(SHY)--061C(ALM)--1361(Ethiopic word space)
 * webkit:   [--------------------------------)        [------------------)       [-]
 * moz:      [--------------------------------)        [---------]                          [-]
 * p2h:      [--------------------------------)        [------------------]       [-]       [-]         [-]
 *
 *         200B(ZWSP)--200C(ZWNJ)--200D(ZWJ)--200E(LRM)--200F(RLM)--2028(LSEP)--2029(PSEP)--202A(LRE)--202E(RL0)--2066(LRI)--2069(PDI)
 * webkit:   [-----------------------------------------------]                                 [----------]
 * moz:      [-]                                  [----------]         [-]         [-]         [----------]         [------------]
 * p2h:      [-----------------------------------------------]         [-]         [-]         [----------]         [------------]
 *
 *         D800(surrogate)--DFFF(surrogate)--FEFF(ZWNBSP)--FFFC(ORC)--FFFE(non-char)--FFFF(non-char)
 * webkit:                                      [-]           [-]
 * moz:
 * p2h:         [------------------]            [-]           [-]          [-----------------]
 *
 * Note: 0xA0 (no-break space) affects word-spacing; and if "white-space:pre" is specified,
 * \n and \r can break line, \t can shift text, so they are considered illegal.
 *
 * Resources (retrieved at 2015-03-16)
 * * webkit
 *   * Avoid querying the font cache for the zero-width space glyph ( https://bugs.webkit.org/show_bug.cgi?id=90673 )
 *   * treatAsZeroWidthSpace( https://github.com/WebKit/webkit/blob/17bbff7400393e9389b40cc84ce005f7cc954680/Source/WebCore/platform/graphics/FontCascade.h#L272 )
 * * mozilla
 *   * IsInvalidChar( http://mxr.mozilla.org/mozilla-central/source/gfx/thebes/gfxTextRun.cpp#1973 )
 *   * IsBidiControl( http://mxr.mozilla.org/mozilla-central/source/intl/unicharutil/util/nsBidiUtils.h#114 )
 * * Character encodings in HTML ( http://en.wikipedia.org/wiki/Character_encodings_in_HTML#HTML_character_references )
 * * CSS Text Spec ( http://dev.w3.org/csswg/css-text/ )
 * * unicode table ( http://unicode-table.com )
 *
 * TODO Web specs? IE?
 *
 */
inline bool is_illegal_unicode(Unicode c)
{
    return (c < 0x20) || (c >= 0x7F && c <= 0xA0) || (c == 0xAD)
            || (c >= 0x300 && c <= 0x36f) // DCRH Combining diacriticals
            || (c >= 0x1ab0 && c <= 0x1aff) // DCRH Combining diacriticals
            || (c >= 0x1dc0 && c <= 0x1dff) // DCRH Combining diacriticals
            || (c >= 0x20d0 && c <= 0x20ff) // DCRH Combining diacriticals
            || (c >= 0xfe20 && c <= 0xfe2f) // DCRH Combining diacriticals
            || (c >= 0x900 && c <= 0x97f) // DCRH Devanagari - Webkit struggles with spacing for these code points
            || (c >= 0xa00 && c <= 0xa7f) // DCRH Gurmukhi - Webkit struggles with spacing for these code points
            || (c == 0x061C) || (c == 0x1361)
            || (c >= 0x200B && c <= 0x200F) || (c == 0x2028) || (c == 0x2029)
            || (c >= 0x202A && c <= 0x202E) || (c >= 0x2066 && c <= 0x2069)
            || (c >= 0xD800 && c <= 0xDFFF) || (c == 0xFEFF) || (c == 0xFFFC)
            || (c == 0xFFFE) || (c == 0xFFFF);
}

Unicode map_to_private(CharCode code);

/* * Try to determine the Unicode value directly from the information in the font */
Unicode unicode_from_font (CharCode code, GfxFont * font);

/*
 * We have to use a single Unicode value to reencode fonts
 * if we got multi-unicode values, it might be expanded ligature, try to restore it
 * if we cannot figure it out at the end, use a private mapping
 */
Unicode check_unicode(Unicode * u, int len, CharCode code, GfxFont * font);


} // namespace pdf2htmlEX

#endif //UNICODE_H__
