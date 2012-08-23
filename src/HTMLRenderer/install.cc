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
#include <fofi/FoFiTrueType.h>

#include "HTMLRenderer.h"
#include "namespace.h"

long long HTMLRenderer::install_font(GfxFont * font)
{
    assert(sizeof(long long) == 2*sizeof(int));
                
    long long fn_id = (font == nullptr) ? 0 : *reinterpret_cast<long long*>(font->getID());

    auto iter = font_name_map.find(fn_id);
    if(iter != font_name_map.end())
        return iter->second.fn_id;

    long long new_fn_id = font_name_map.size(); 

    font_name_map.insert(make_pair(fn_id, FontInfo({new_fn_id})));

    if(font == nullptr)
    {
        export_remote_default_font(new_fn_id);
        return new_fn_id;
    }

    if(param->debug)
    {
        cerr << "Install font: (" << (font->getID()->num) << ' ' << (font->getID()->gen) << ") -> " << format("f%|1$x|")%new_fn_id << endl;
    }

    if(font->getType() == fontType3) {
        cerr << "Type 3 fonts are unsupported and will be rendered as Image" << endl;
        export_remote_default_font(new_fn_id);
        return new_fn_id;
    }
    if(font->getWMode()) {
        cerr << "Writing mode is unsupported and will be rendered as Image" << endl;
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
                    string suffix = dump_embedded_font(font, new_fn_id);
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
      
    return new_fn_id;

}

// TODO
// add a new function and move to text.cc
void HTMLRenderer::install_embedded_font(GfxFont * font, const string & suffix, long long fn_id)
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
    
    string fn = (format("f%|1$x|") % fn_id).str();

    path script_path = tmp_dir / FONTFORGE_SCRIPT_FILENAME;
    ofstream script_fout(script_path, ofstream::binary);
    add_tmp_file(FONTFORGE_SCRIPT_FILENAME);

    script_fout << format("Open(%1%, 1)") % (tmp_dir / (fn + suffix)) << endl;

    auto ctu = font->getToUnicode();
    int * code2GID = nullptr;
    int code2GID_len = 0;
    if(ctu)
    {
        int maxcode = 0;

        if(!font->isCIDFont())
        {
            if(suffix == ".ttf")
            {
                maxcode = 0xff;
                script_fout << "Reencode(\"original\")" << endl;
                int buflen;
                char * buf = nullptr;
                if((buf = font->readEmbFontFile(xref, &buflen)))
                {
                    FoFiTrueType *fftt = nullptr;
                    if((fftt = FoFiTrueType::make(buf, buflen)))
                    {
                        code2GID = dynamic_cast<Gfx8BitFont*>(font)->getCodeToGIDMap(fftt);
                        code2GID_len = 256;
                        delete fftt;
                    }
                    gfree(buf);
                }
            }
            else
            {
                // don't reencode non-ttf 8bit fonts with ToUnicode
                maxcode = 0;
                script_fout << "Reencode(\"unicode\")" << endl;
            }
        }
        else
        {
            maxcode = 0xffff;
            if(suffix == ".ttf")
            {
                script_fout << "Reencode(\"original\")" << endl;
    
                GfxCIDFont * _font = dynamic_cast<GfxCIDFont*>(font);

                // code2GID has been stored for embedded CID fonts
                code2GID = _font->getCIDToGID();
                code2GID_len = _font->getCIDToGIDLen();
            }
            else
            {
                script_fout << "CIDFlatten()" << endl;
            }
        }

        if(maxcode > 0)
        {
            ofstream map_fout(tmp_dir / (fn + ".encoding"));
            add_tmp_file(fn+".encoding");

            int cnt = 0;
            for(int i = 0; i <= maxcode; ++i)
            {
                Unicode * u;
                auto n = ctu->mapToUnicode(i, &u);
                // not sure what to do when n > 1
                if(n > 0)
                {
                    ++cnt;
                    map_fout << format("0x%|1$X|") % ((code2GID && (i < code2GID_len))? code2GID[i] : i);
                    for(int j = 0; j < n; ++j)
                        map_fout << format(" 0x%|1$X|") % u[j];
                    map_fout << format(" # 0x%|1$X|") % i << endl;
                }
            }

            if(cnt > 0)
            {
                script_fout << format("LoadEncodingFile(%1%, \"%2%\")") % (tmp_dir / (fn+".encoding")) % fn << endl;
                script_fout << format("Reencode(\"%1%\", 1)") % fn << endl;
            }
        }

        ctu->decRefCnt();
    }

    script_fout << format("Generate(%1%)") % ((param->single_html ? tmp_dir : dest_dir) / (fn+".ttf")) << endl;
    if(param->single_html)
        add_tmp_file(fn+".ttf");

    if(system((boost::format("fontforge -script %1% 2>%2%") % script_path % (tmp_dir / NULL_FILENAME)).str().c_str()) != 0)
        cerr << "Warning: fontforge failed." << endl;

    add_tmp_file(NULL_FILENAME);

    export_remote_font(fn_id, ".ttf", "truetype", font);
}

void HTMLRenderer::install_base_font(GfxFont * font, GfxFontLoc * font_loc, long long fn_id)
{
    string psname(font_loc->path->getCString());
    string basename = psname.substr(0, psname.find('-'));
    string cssfont;
    auto iter = BASE_14_FONT_CSS_FONT_MAP.find(basename);
    if(iter == BASE_14_FONT_CSS_FONT_MAP.end())
    {
        cerr << "PS Font: " << basename << " not found in the base 14 font map" << endl;
        cssfont = "";
    }
    else
        cssfont = iter->second;

    export_local_font(fn_id, font, psname, cssfont);
}

void HTMLRenderer::install_external_font( GfxFont * font, long long fn_id)
{
    string fontname(font->getName()->getCString());

    // resolve bad encodings in GB
    auto iter = GB_ENCODED_FONT_NAME_MAP.find(fontname); 
    if(iter != GB_ENCODED_FONT_NAME_MAP.end())
    {
        fontname = iter->second;
        cerr << "Warning: workaround for font names in bad encodings." << endl;
    }

    export_local_font(fn_id, font, fontname, "");
}
    
long long HTMLRenderer::install_font_size(double font_size)
{
    auto iter = font_size_map.lower_bound(font_size - EPS);
    if((iter != font_size_map.end()) && (_equal(iter->first, font_size)))
        return iter->second;

    long long new_fs_id = font_size_map.size();
    font_size_map.insert(make_pair(font_size, new_fs_id));
    export_font_size(new_fs_id, font_size);
    return new_fs_id;
}

long long HTMLRenderer::install_transform_matrix(const double * tm)
{
    TM m(tm);
    auto iter = transform_matrix_map.lower_bound(m);
    if((iter != transform_matrix_map.end()) && (m == (iter->first)))
        return iter->second;

    long long new_tm_id = transform_matrix_map.size();
    transform_matrix_map.insert(make_pair(m, new_tm_id));
    export_transform_matrix(new_tm_id, tm);
    return new_tm_id;
}

long long HTMLRenderer::install_letter_space(double letter_space)
{
    auto iter = letter_space_map.lower_bound(letter_space - EPS);
    if((iter != letter_space_map.end()) && (_equal(iter->first, letter_space)))
        return iter->second;

    long long new_ls_id = letter_space_map.size();
    letter_space_map.insert(make_pair(letter_space, new_ls_id));
    export_letter_space(new_ls_id, letter_space);
    return new_ls_id;
}

long long HTMLRenderer::install_word_space(double word_space)
{
    auto iter = word_space_map.lower_bound(word_space - EPS);
    if((iter != word_space_map.end()) && (_equal(iter->first, word_space)))
        return iter->second;

    long long new_ws_id = word_space_map.size();
    word_space_map.insert(make_pair(word_space, new_ws_id));
    export_word_space(new_ws_id, word_space);
    return new_ws_id;
}

long long HTMLRenderer::install_color(const GfxRGB * rgb)
{
    const GfxRGB & c = *rgb;
    auto iter = color_map.lower_bound(c);
    if((iter != color_map.end()) && (c == (iter->first)))
        return iter->second;

    long long new_color_id = color_map.size();
    color_map.insert(make_pair(c, new_color_id));
    export_color(new_color_id, rgb);
    return new_color_id;
}

long long HTMLRenderer::install_whitespace(double ws_width, double & actual_width)
{
    // ws_width is already mulitpled by draw_scale
    auto iter = whitespace_map.lower_bound(ws_width - param->h_eps);
    if((iter != whitespace_map.end()) && (abs(iter->first - ws_width) < param->h_eps))
    {
        actual_width = iter->first;
        return iter->second;
    }

    actual_width = ws_width;
    long long new_ws_id = whitespace_map.size();
    whitespace_map.insert(make_pair(ws_width, new_ws_id));
    export_whitespace(new_ws_id, ws_width);
    return new_ws_id;
}

