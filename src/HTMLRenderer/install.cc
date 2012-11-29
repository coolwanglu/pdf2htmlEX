/*
 * install.cc
 *
 * maintaining all known styles
 *
 * by WangLu
 * 2012.08.14
 */

#include <iostream>
#include <cmath>
#include <algorithm>
#include <functional>

#include <GlobalParams.h>

#include "Param.h"
#include "HTMLRenderer.h"
#include "namespace.h"
#include "util.h"
#include "export.h"


namespace pdf2htmlEX {

using std::abs;
using namespace std::placeholders;

namespace
{

template <class Map, class Export>
long long install_double( Map& items, double val, Export fn_export, double eps = EPS, double* actual_val = nullptr )
{
    auto iter = items.lower_bound( val - eps );
    if((iter != items.end()) && (_equal(iter->first, val, eps)))
	{
		if ( actual_val )
			*actual_val = iter->first;
        return iter->second;
	}

	if ( actual_val )
		*actual_val = val;
    long long const new_id = items.size();
    items.insert( make_pair( val, new_id ) );
    fn_export( new_id, val );
    return new_id;
}
    

} // anonymous namespace

const FontInfo * HTMLRenderer::install_font(GfxFont * font)
{
    assert(sizeof(long long) == 2*sizeof(int));
                
    long long fn_id = (font == nullptr) ? 0 : hash_ref(font->getID());

    auto iter = font_name_map.find(fn_id);
    if(iter != font_name_map.end())
        return &(iter->second);

    long long new_fn_id = font_name_map.size(); 

    auto cur_info_iter = font_name_map.insert(make_pair(fn_id, FontInfo({new_fn_id, true}))).first;

    if(font == nullptr)
    {
        export_remote_default_font(css_fout, new_fn_id);
        return &(cur_info_iter->second);
    }

    cur_info_iter->second.ascent = font->getAscent();
    cur_info_iter->second.descent = font->getDescent();

    if(param->debug)
    {
        cerr << "Install font: (" << (font->getID()->num) << ' ' << (font->getID()->gen) << ") -> " << "f" << hex << new_fn_id << dec << endl;
    }

    if(font->getType() == fontType3) {
        cerr << "Type 3 fonts are unsupported and will be rendered as Image" << endl;
        export_remote_default_font(css_fout, new_fn_id);
        return &(cur_info_iter->second);
    }
    if(font->getWMode()) {
        cerr << "Writing mode is unsupported and will be rendered as Image" << endl;
        export_remote_default_font(css_fout, new_fn_id);
        return &(cur_info_iter->second);
    }

    auto * font_loc = font->locateFont(xref, gTrue);
    if(font_loc != nullptr)
    {
        switch(font_loc -> locType)
        {
            case gfxFontLocEmbedded:
                install_embedded_font(font, cur_info_iter->second);
                break;
            case gfxFontLocExternal:
                install_external_font(font, cur_info_iter->second);
                break;
            case gfxFontLocResident:
                install_base_font(font, font_loc, cur_info_iter->second);
                break;
            default:
                cerr << "TODO: other font loc" << endl;
                export_remote_default_font(css_fout, new_fn_id);
                break;
        }      
        delete font_loc;
    }
    else
    {
        export_remote_default_font( css_fout, new_fn_id);
    }
      
    return &(cur_info_iter->second);
}

void HTMLRenderer::install_embedded_font(GfxFont * font, FontInfo & info)
{
    auto path = dump_embedded_font(font, info.id);

    if(path != "")
    {
        embed_font(path, font, info);
        export_remote_font(info, param->font_suffix, param->font_format, font);
    }
    else
    {
        export_remote_default_font(css_fout, info.id);
    }
}

void HTMLRenderer::install_base_font(GfxFont * font, GfxFontLoc * font_loc, FontInfo & info)
{
    string psname(font_loc->path->getCString());
    string basename = psname.substr(0, psname.find('-'));

    GfxFontLoc * localfontloc = font->locateFont(xref, gFalse);
    if(param->embed_base_font)
    {
        if(localfontloc != nullptr)
        {
            embed_font(localfontloc->path->getCString(), font, info);
            export_remote_font(info, param->font_suffix, param->font_format, font);
            delete localfontloc;
            return;
        }
        else
        {
            cerr << "Cannot embed base font: f" << hex << info.id << dec << ' ' << psname << endl;
            // fallback to exporting by name
        }

    }

    string cssfont;
    auto iter = BASE_14_FONT_CSS_FONT_MAP.find(basename);
    if(iter == BASE_14_FONT_CSS_FONT_MAP.end())
    {
        cerr << "PS Font: " << basename << " not found in the base 14 font map" << endl;
        cssfont = "";
    }
    else
        cssfont = iter->second;

    // still try to get an idea of read ascent/descent
    if(localfontloc != nullptr)
    {
        // fill in ascent/descent only, do not embed
        embed_font(string(localfontloc->path->getCString()), font, info, true);
        delete localfontloc;
    }
    else
    {
        info.ascent = font->getAscent();
        info.descent = font->getDescent();
    }

    export_local_font(css_fout, info, font, psname, cssfont);
}

void HTMLRenderer::install_external_font(GfxFont * font, FontInfo & info)
{
    string fontname(font->getName()->getCString());

    // resolve bad encodings in GB
    auto iter = GB_ENCODED_FONT_NAME_MAP.find(fontname); 
    if(iter != GB_ENCODED_FONT_NAME_MAP.end())
    {
        fontname = iter->second;
        cerr << "Warning: workaround for font names in bad encodings." << endl;
    }

    GooString gfn(fontname.c_str());
    GfxFontLoc * localfontloc = font->locateFont(xref, gFalse);

    if(param->embed_external_font)
    {
        if(localfontloc != nullptr)
        {
            embed_font(string(localfontloc->path->getCString()), font, info);
            export_remote_font(info, param->font_suffix, param->font_format, font);
            delete localfontloc;
            return;
        }
        else
        {
            cerr << "Cannot embed external font: f" << hex << info.id << dec << ' ' << fontname << endl;
            // fallback to exporting by name
        }
    }

    // still try to get an idea of read ascent/descent
    if(localfontloc != nullptr)
    {
        // fill in ascent/descent only, do not embed
        embed_font(string(localfontloc->path->getCString()), font, info, true);
        delete localfontloc;
    }
    else
    {
        info.ascent = font->getAscent();
        info.descent = font->getDescent();
    }

    export_local_font(css_fout, info, font, fontname, "");
}

long long HTMLRenderer::install_font_size(double font_size)
{
	return install_double( font_size_map, font_size, std::bind( export_font_size, std::ref(css_fout), _1, _2 ) );
}

long long HTMLRenderer::install_transform_matrix(const double * tm)
{
    Matrix m;
    memcpy(m.m, tm, sizeof(m.m));

    auto iter = transform_matrix_map.lower_bound(m);
    if((iter != transform_matrix_map.end()) && (_tm_equal(m.m, iter->first.m, 4)))
        return iter->second;

    long long new_tm_id = transform_matrix_map.size();
    transform_matrix_map.insert(make_pair(m, new_tm_id));
    export_transform_matrix( css_fout, new_tm_id, tm);
    return new_tm_id;
}

long long HTMLRenderer::install_letter_space(double letter_space)
{
	return install_double( letter_space_map, letter_space, std::bind( export_letter_space, std::ref(css_fout), _1, _2 ) );
}

long long HTMLRenderer::install_word_space(double word_space)
{
	return install_double( word_space_map, word_space, std::bind( export_word_space, std::ref(css_fout), _1, _2 ) );
}

long long HTMLRenderer::install_color(const GfxRGB * rgb)
{
    const GfxRGB & c = *rgb;
    auto iter = color_map.find(c);
    if(iter != color_map.end())
        return iter->second;

    long long new_color_id = color_map.size();
    color_map.insert(make_pair(c, new_color_id));
    export_color(css_fout, new_color_id, rgb);
    return new_color_id;
}

long long HTMLRenderer::install_whitespace(double ws_width, double & actual_width)
{
    // ws_width is already mulitpled by draw_scale
	return install_double( whitespace_map, ws_width, std::bind( export_whitespace, std::ref(css_fout), _1, _2 ), param->h_eps, &actual_width );
}

long long HTMLRenderer::install_rise(double rise)
{
	return install_double( rise_map, rise, std::bind( export_rise, std::ref(css_fout), _1, _2 ), param->v_eps );
}

long long HTMLRenderer::install_height(double height)
{
	return install_double( height_map, height, std::bind( export_height, std::ref(css_fout), _1, _2 ) );
}

} // namespace pdf2htmlEX
