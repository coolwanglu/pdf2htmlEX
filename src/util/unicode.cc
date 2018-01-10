/*
 * Unicode manipulation functions
 *
 * Copyright (C) 2012-2014 Lu Wang <coolwanglu@gmail.com>
 */

#include <iostream>

#include <GlobalParams.h>

#include "pdf2htmlEX-config.h"

#include "unicode.h"

namespace pdf2htmlEX {

using std::cerr;
using std::endl;
using std::ostream;

Unicode map_to_private(CharCode code)
{
    Unicode private_mapping = (Unicode)(code + 0xE600); // DCRH: Stupid mobile safari uses code points in 0xe000 - 0xe5ff range to switch to emoji font
    if(private_mapping > 0xF65F) // DCRH: More emoji-avoiding for mobile safari (see http://www.fileformat.info/info/unicode/block/private_use_area/utf8test.htm)
    {
        private_mapping = (Unicode)((private_mapping - 0xF65F) + 0xF0000);
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
        auto * font2 = dynamic_cast<Gfx8BitFont*>(font);
        assert(font2 != nullptr);
        char * cname = font2->getCharName(code);
        // may be untranslated ligature
        if(cname)
        {
            Unicode ou = globalParams->mapNameToUnicodeText(cname);
            if(!is_illegal_unicode(ou))
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
        if(!is_illegal_unicode(*u))
            return *u;
    }

    return unicode_from_font(code, font);
}

} //namespace pdf2htmlEX
