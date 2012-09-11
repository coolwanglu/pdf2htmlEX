/*
 * FontPreprocessor.h
 *
 * Check used codes for each font
 *
 * by WangLu
 * 2012.09.07
 */

#include <cstring>

#include <GfxState.h>
#include <GfxFont.h>

#include "FontPreprocessor.h"
#include "util.h"

namespace pdf2htmlEX {

FontPreprocessor::FontPreprocessor(void)
    : cur_font_id(0)
    , cur_code_map(nullptr)
{ }

FontPreprocessor::~FontPreprocessor(void)
{
    for(auto iter = code_maps.begin(); iter != code_maps.end(); ++iter)
        delete [] iter->second;
}

void FontPreprocessor::drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen)
{
    GfxFont * font = state->getFont();
    if(!font) return;

    long long fn_id = hash_ref(font->getID());

    if(fn_id != cur_font_id)
    {
        cur_font_id = fn_id;
        auto p = code_maps.insert(std::make_pair(cur_font_id, (char*)nullptr));
        if(p.second)
        {
            // this is a new font
            int len = font->isCIDFont() ? 0x10000 : 0x100;
            p.first->second = new char [len];
            memset(p.first->second, 0, len * sizeof(char));
        }

        cur_code_map = p.first->second;
    }

    cur_code_map[code] = 1;
}

const char * FontPreprocessor::get_code_map (long long font_id) const
{
    auto iter = code_maps.find(font_id);
    return (iter == code_maps.end()) ? nullptr : (iter->second);
}

} // namespace pdf2htmlEX
