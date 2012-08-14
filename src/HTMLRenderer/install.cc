/*
 * install.cc
 *
 * maintaining all known styles
 *
 * by WangLu
 * 2012.08.14
 */

#include <iostream>

#include <boost/format.hpp>

#include <CharCodeToUnicode.h>

#include "HTMLRenderer.h"

long long HTMLRenderer::install_font(GfxFont * font)
{
    assert(sizeof(long long) == 2*sizeof(int));
                
    long long fn_id = (font == nullptr) ? 0 : *reinterpret_cast<long long*>(font->getID());

    auto iter = font_name_map.find(fn_id);
    if(iter != font_name_map.end())
        return iter->second.fn_id;

    long long new_fn_id = font_name_map.size(); 

    font_name_map.insert(std::make_pair(fn_id, FontInfo({new_fn_id})));

    if(font == nullptr)
    {
        export_remote_default_font(new_fn_id);
        return new_fn_id;
    }

    if(param->debug)
    {
        std::cerr << "Install font: (" << (font->getID()->num) << ' ' << (font->getID()->gen) << ") -> " << boost::format("f%|1$x|")%new_fn_id << std::endl;
    }

    if(font->getType() == fontType3) {
        std::cerr << "Type 3 fonts are unsupported and will be rendered as Image" << std::endl;
        export_remote_default_font(new_fn_id);
        return new_fn_id;
    }
    if(font->getWMode()) {
        std::cerr << "Writing mode is unsupported and will be rendered as Image" << std::endl;
        export_remote_default_font(new_fn_id);
        return new_fn_id;
    }

    auto * font_loc = font->locateFont(xref, gTrue);
    if(font_loc != nullptr)
    {
        switch(font_loc -> locType)
        {
            case gfxFontLocEmbedded:
                {
                    std::string suffix = dump_embedded_font(font, new_fn_id);
                    if(suffix != "")
                    {
                        install_embedded_font(font, suffix, new_fn_id);
                    }
                    else
                    {
                        export_remote_default_font(new_fn_id);
                    }
                }
                break;
            case gfxFontLocExternal:
                install_external_font(font, new_fn_id);
                break;
            case gfxFontLocResident:
                install_base_font(font, font_loc, new_fn_id);
                break;
            default:
                std::cerr << "TODO: other font loc" << std::endl;
                export_remote_default_font(new_fn_id);
                break;
        }      
        delete font_loc;
    }
    else
    {
        export_remote_default_font(new_fn_id);
    }
      
    return new_fn_id;

}

std::string HTMLRenderer::dump_embedded_font (GfxFont * font, long long fn_id)
{
    // mupdf consulted
    
    Object ref_obj, font_obj, font_obj2, fontdesc_obj;
    Object obj, obj1, obj2;
    Dict * dict = nullptr;

    std::string suffix, subtype;

    char buf[1024];
    int len;

    ofstream outf;

    auto * id = font->getID();
    ref_obj.initRef(id->num, id->gen);
    ref_obj.fetch(xref, &font_obj);
    ref_obj.free();

    if(!font_obj.isDict())
    {
        std::cerr << "Font object is not a dictionary" << std::endl;
        goto err;
    }

    dict = font_obj.getDict();
    if(dict->lookup("DescendantFonts", &font_obj2)->isArray())
    {
        if(font_obj2.arrayGetLength() == 0)
        {
            std::cerr << "Warning: empty DescendantFonts array" << std::endl;
        }
        else
        {
            if(font_obj2.arrayGetLength() > 1)
                std::cerr << "TODO: multiple entries in DescendantFonts array" << std::endl;

            if(font_obj2.arrayGet(0, &obj2)->isDict())
            {
                dict = obj2.getDict();
            }
        }
    }

    if(!dict->lookup("FontDescriptor", &fontdesc_obj)->isDict())
    {
        std::cerr << "Cannot find FontDescriptor " << std::endl;
        goto err;
    }

    dict = fontdesc_obj.getDict();
    
    if(dict->lookup("FontFile3", &obj)->isStream())
    {
        if(obj.streamGetDict()->lookup("Subtype", &obj1)->isName())
        {
            subtype = obj1.getName();
            if(subtype == "Type1C")
            {
                suffix = ".cff";
            }
            else if (subtype == "CIDFontType0C")
            {
                suffix = ".cid";
            }
            else
            {
                std::cerr << "Unknown subtype: " << subtype << std::endl;
                goto err;
            }
        }
        else
        {
            std::cerr << "Invalid subtype in font descriptor" << std::endl;
            goto err;
        }
    }
    else if (dict->lookup("FontFile2", &obj)->isStream())
    { 
        suffix = ".ttf";
    }
    else if (dict->lookup("FontFile", &obj)->isStream())
    {
        suffix = ".ttf";
    }
    else
    {
        std::cerr << "Cannot find FontFile for dump" << std::endl;
        goto err;
    }

    if(suffix == "")
    {
        std::cerr << "Font type unrecognized" << std::endl;
        goto err;
    }

    obj.streamReset();
    outf.open((boost::format("%1%/f%|2$x|%3%")%TMP_DIR%fn_id%suffix).str().c_str(), ofstream::binary);
    while((len = obj.streamGetChars(1024, (Guchar*)buf)) > 0)
    {
        outf.write(buf, len);
    }
    outf.close();
    obj.streamClose();

err:
    obj2.free();
    obj1.free();
    obj.free();

    fontdesc_obj.free();
    font_obj2.free();
    font_obj.free();
    return suffix;
}

void HTMLRenderer::install_embedded_font(GfxFont * font, const std::string & suffix, long long fn_id)
{
    // TODO Should use standard way to handle CID fonts
    /*
     * How it works:
     *
     * 1.dump the font file directly from the font descriptor and put the glyphs into the correct slots
     *
     * for nonCID
     * nothing need to do
     *
     * for CID + nonTrueType
     * Flatten the font 
     *
     * for CID Truetype
     * Just use glyph order, and later we'll map GID (instead of char code) to Unicode
     *
     *
     * 2. map charcode (or GID for CID truetype) to Unicode
     * 
     * generate an encoding file and let fontforge handle it.
     */
    
    std::string fn = (boost::format("f%|1$x|") % fn_id).str();

    fontscript_fout << boost::format("Open(\"%1%/%2%%3%\",1)") % TMP_DIR % fn % suffix << endl;

    auto ctu = font->getToUnicode();
    int * code2GID = nullptr;
    if(ctu)
    {
        // TODO: ctu could be CID2Unicode for CID fonts
        int maxcode = 0;

        if(!font->isCIDFont())
        {
            maxcode = 0xff;
        }
        else
        {
            maxcode = 0xffff;
            if(suffix != ".ttf")
            {
                fontscript_fout << "CIDFlatten()" << endl;
            }
            else
            {
                fontscript_fout << boost::format("Reencode(\"original\")") << endl;
                int len; 
                // code2GID has been stored for embedded CID fonts
                code2GID = dynamic_cast<GfxCIDFont*>(font)->getCodeToGIDMap(nullptr, &len);
            }
        }

        if(maxcode > 0)
        {
            ofstream map_fout((boost::format("%1%/%2%.encoding") % TMP_DIR % fn).str().c_str());
            int cnt = 0;
            for(int i = 0; i <= maxcode; ++i)
            {
                Unicode * u;
                auto n = ctu->mapToUnicode(i, &u);
                // not sure what to do when n > 1
                if(n > 0)
                {
                    ++cnt;
                    map_fout << boost::format("0x%|1$X|") % (code2GID ? code2GID[i] : i);
                    for(int j = 0; j < n; ++j)
                        map_fout << boost::format(" 0x%|1$X|") % u[j];
                    map_fout << boost::format(" # 0x%|1$X|") % i << endl;
                }
            }

            if(cnt > 0)
            {
                fontscript_fout << boost::format("LoadEncodingFile(\"%1%/%2%.encoding\", \"%2%\")") % TMP_DIR % fn << endl;
                fontscript_fout << boost::format("Reencode(\"%1%\", 1)") % fn << endl;
            }
        }

        ctu->decRefCnt();
    }

    fontscript_fout << boost::format("Generate(\"%1%.ttf\")") % fn << endl;

    export_remote_font(fn_id, ".ttf", "truetype", font);
}

void HTMLRenderer::install_base_font(GfxFont * font, GfxFontLoc * font_loc, long long fn_id)
{
    std::string psname(font_loc->path->getCString());
    string basename = psname.substr(0, psname.find('-'));
    string cssfont;
    auto iter = BASE_14_FONT_CSS_FONT_MAP.find(basename);
    if(iter == BASE_14_FONT_CSS_FONT_MAP.end())
    {
        std::cerr << "PS Font: " << basename << " not found in the base 14 font map" << std::endl;
        cssfont = "";
    }
    else
        cssfont = iter->second;

    export_local_font(fn_id, font, psname, cssfont);
}

void HTMLRenderer::install_external_font( GfxFont * font, long long fn_id)
{
    std::string fontname(font->getName()->getCString());

    // resolve bad encodings in GB
    auto iter = GB_ENCODED_FONT_NAME_MAP.find(fontname); 
    if(iter != GB_ENCODED_FONT_NAME_MAP.end())
    {
        fontname = iter->second;
        std::cerr << "Warning: workaround for font names in bad encodings." << std::endl;
    }

    export_local_font(fn_id, font, fontname, "");
}
    
long long HTMLRenderer::install_font_size(double font_size)
{
    auto iter = font_size_map.lower_bound(font_size - EPS);
    if((iter != font_size_map.end()) && (_equal(iter->first, font_size)))
        return iter->second;

    long long new_fs_id = font_size_map.size();
    font_size_map.insert(std::make_pair(font_size, new_fs_id));
    export_font_size(new_fs_id, font_size);
    return new_fs_id;
}

long long HTMLRenderer::install_whitespace(double ws_width, double & actual_width)
{
    auto iter = whitespace_map.lower_bound(ws_width - param->h_eps);
    if((iter != whitespace_map.end()) && (std::abs(iter->first - ws_width) < param->h_eps))
    {
        actual_width = iter->first;
        return iter->second;
    }

    actual_width = ws_width;
    long long new_ws_id = whitespace_map.size();
    whitespace_map.insert(std::make_pair(ws_width, new_ws_id));
    export_whitespace(new_ws_id, ws_width);
    return new_ws_id;
}

long long HTMLRenderer::install_transform_matrix(const double * tm)
{
    TM m(tm);
    auto iter = transform_matrix_map.lower_bound(m);
    if((iter != transform_matrix_map.end()) && (m == (iter->first)))
        return iter->second;

    long long new_tm_id = transform_matrix_map.size();
    transform_matrix_map.insert(std::make_pair(m, new_tm_id));
    export_transform_matrix(new_tm_id, tm);
    return new_tm_id;
}

long long HTMLRenderer::install_color(const GfxRGB * rgb)
{
    Color c(rgb);
    auto iter = color_map.lower_bound(c);
    if((iter != color_map.end()) && (c == (iter->first)))
        return iter->second;

    long long new_color_id = color_map.size();
    color_map.insert(std::make_pair(c, new_color_id));
    export_color(new_color_id, rgb);
    return new_color_id;
}

