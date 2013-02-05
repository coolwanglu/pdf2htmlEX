/*
 * font.cc
 *
 * Font processing
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <iostream>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <cctype>

#include <GlobalParams.h>

#include "Param.h"
#include "HTMLRenderer.h"
#include "util/namespace.h"
#include "util/math.h"
#include "util/misc.h"
#include "util/base64stream.h"

namespace pdf2htmlEX {

using std::abs;
using std::cerr;
using std::endl;

const FontInfo * HTMLRenderer::install_font(GfxFont * font)
{
    assert(sizeof(long long) == 2*sizeof(int));
                
    long long fn_id = (font == nullptr) ? 0 : hash_ref(font->getID());

    auto iter = font_name_map.find(fn_id);
    if(iter != font_name_map.end())
        return &(iter->second);

    long long new_fn_id = font_name_map.size(); 

    auto cur_info_iter = font_name_map.insert(make_pair(fn_id, FontInfo())).first;

    FontInfo & new_font_info = cur_info_iter->second;
    new_font_info.id = new_fn_id;
    new_font_info.use_tounicode = true;

    if(font == nullptr)
    {
        new_font_info.ascent = 0;
        new_font_info.descent = 0;
        new_font_info.is_type3 = false;

        export_remote_default_font(new_fn_id);

        return &(new_font_info);
    }

    new_font_info.ascent = font->getAscent();
    new_font_info.descent = font->getDescent();
    new_font_info.is_type3 = (font->getType() == fontType3);

    if(param->debug)
    {
        cerr << "Install font: (" << (font->getID()->num) << ' ' << (font->getID()->gen) << ") -> " << "f" << hex << new_fn_id << dec << endl;
    }

    if(font->getType() == fontType3) {
        cerr << "Type 3 fonts are unsupported and will be rendered as Image" << endl;
        export_remote_default_font(new_fn_id);
        return &(cur_info_iter->second);
    }
    if(font->getWMode()) {
        cerr << "Writing mode is unsupported and will be rendered as Image" << endl;
        export_remote_default_font(new_fn_id);
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
                export_remote_default_font(new_fn_id);
                break;
        }      
        delete font_loc;
    }
    else
    {
        export_remote_default_font(new_fn_id);
    }
      
    return &(cur_info_iter->second);
}

void HTMLRenderer::install_embedded_font(GfxFont * font, FontInfo & info)
{
    auto path = dump_embedded_font(font, info.id);

    if(path != "")
    {
        embed_font(path, font, info);
        export_remote_font(info, param->font_suffix, font);
    }
    else
    {
        export_remote_default_font(info.id);
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
            export_remote_font(info, param->font_suffix, font);
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

    export_local_font(info, font, psname, cssfont);
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

    GfxFontLoc * localfontloc = font->locateFont(xref, gFalse);

    if(param->embed_external_font)
    {
        if(localfontloc != nullptr)
        {
            embed_font(string(localfontloc->path->getCString()), font, info);
            export_remote_font(info, param->font_suffix, font);
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

    export_local_font(info, font, fontname, "");
}

void HTMLRenderer::export_remote_font(const FontInfo & info, const string & suffix, GfxFont * font)
{
    string mime_type, format;
    if(suffix == ".ttf")
    {
        format = "truetype";
        mime_type = "application/x-font-ttf";
    }
    else if(suffix == ".otf")
    {
        format = "opentype";
        mime_type = "application/x-font-otf";
    }
    else if(suffix == ".woff")
    {
        format = "woff";
        mime_type = "application/font-woff";
    }
    else if(suffix == ".eot")
    {
        format = "embedded-opentype";
        mime_type = "application/vnd.ms-fontobject";
    }
    else if(suffix == ".svg")
    {
        format = "svg";
        mime_type = "image/svg+xml";
    }
    else
    {
        cerr << "Warning: unknown font suffix: " << suffix << endl;
    }

    f_css.fs << "@font-face{"
             << "font-family:f" << info.id << ";"
             << "src:url(";

    {
        auto fn = str_fmt("f%llx%s", info.id, suffix.c_str());
        if(param->single_html)
        {
            auto path = param->tmp_dir + "/" + (char*)fn;
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw "Cannot locate font file: " + path;
            f_css.fs << "'data:font/" + mime_type + ";base64," << base64stream(fin) << "'";
        }
        else
        {
            f_css.fs << (char*)fn;
        }
    }

    f_css.fs << ")"
             << "format(\"" << format << "\");"
             << "}" // end of @font-face
             << ".f" << info.id << "{"
             << "font-family:f" << info.id << ";"
             << "line-height:" << round(info.ascent - info.descent) << ";"
             << "font-style:normal;"
             << "font-weight:normal;"
             << "visibility:visible;"
             << "}" // end of .f
             << endl;
}

static string general_font_family(GfxFont * font)
{
    if(font->isFixedWidth())
        return "monospace";
    else if (font->isSerif())
        return "serif";
    else
        return "sans-serif";
}

// TODO: this function is called when some font is unable to process, may use the name there as a hint
void HTMLRenderer::export_remote_default_font(long long fn_id) 
{
    f_css.fs << ".f" << fn_id << "{font-family:sans-serif;visibility:hidden;}" << endl;
}

void HTMLRenderer::export_local_font(const FontInfo & info, GfxFont * font, const string & original_font_name, const string & cssfont) 
{
    f_css.fs << ".f" << info.id << "{";
    f_css.fs << "font-family:" << ((cssfont == "") ? (original_font_name + "," + general_font_family(font)) : cssfont) << ";";

    string fn = original_font_name;
    for(auto iter = fn.begin(); iter != fn.end(); ++iter)
        *iter = tolower(*iter);

    if(font->isBold() || (fn.find("bold") != string::npos))
        f_css.fs << "font-weight:bold;";
    else
        f_css.fs << "font-weight:normal;";

    if(fn.find("oblique") != string::npos)
        f_css.fs << "font-style:oblique;";
    else if(font->isItalic() || (fn.find("italic") != string::npos))
        f_css.fs << "font-style:italic;";
    else
        f_css.fs << "font-style:normal;";

    f_css.fs << "line-height:" << round(info.ascent - info.descent) << ";";

    f_css.fs << "visibility:visible;";

    f_css.fs << "}" << endl;
}

} //namespace pdf2hmlEX
