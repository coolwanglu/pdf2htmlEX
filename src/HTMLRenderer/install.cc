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

#include <boost/format.hpp>

#include <CharCodeToUnicode.h>
#include <fofi/FoFiTrueType.h>

#include "HTMLRenderer.h"
#include "namespace.h"

using std::all_of;

FontInfo HTMLRenderer::install_font(GfxFont * font)
{
    assert(sizeof(long long) == 2*sizeof(int));
                
    long long fn_id = (font == nullptr) ? 0 : *reinterpret_cast<long long*>(font->getID());

    auto iter = font_name_map.find(fn_id);
    if(iter != font_name_map.end())
        return iter->second;

    long long new_fn_id = font_name_map.size(); 

    auto cur_info_iter = font_name_map.insert(make_pair(fn_id, FontInfo({new_fn_id, true}))).first;

    if(font == nullptr)
    {
        export_remote_default_font(new_fn_id);
        return cur_info_iter->second;
    }

    cur_info_iter->second.ascent = font->getAscent();
    cur_info_iter->second.descent = font->getDescent();

    if(param->debug)
    {
        cerr << "Install font: (" << (font->getID()->num) << ' ' << (font->getID()->gen) << ") -> " << format("f%|1$x|")%new_fn_id << endl;
    }

    if(font->getType() == fontType3) {
        cerr << "Type 3 fonts are unsupported and will be rendered as Image" << endl;
        export_remote_default_font(new_fn_id);
        return cur_info_iter->second;
    }
    if(font->getWMode()) {
        cerr << "Writing mode is unsupported and will be rendered as Image" << endl;
        export_remote_default_font(new_fn_id);
        return cur_info_iter->second;
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
                        install_embedded_font(font, suffix, new_fn_id, cur_info_iter->second);
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
      
    return cur_info_iter->second;
}

// TODO
// add a new function and move to text.cc
void HTMLRenderer::install_embedded_font(GfxFont * font, const string & suffix, long long fn_id, FontInfo & info)
{
    string fn = (format("f%|1$x|") % fn_id).str();

    path script_path = tmp_dir / (fn + ".pe");
    ofstream script_fout(script_path, ofstream::binary);
    add_tmp_file(fn+".pe");

    script_fout << format("Open(%1%, 1)") % (tmp_dir / (fn + suffix)) << endl;

    int * code2GID = nullptr;
    int code2GID_len = 0;
    int maxcode = 0;

    Gfx8BitFont * font_8bit = nullptr;

    /*
     * Step 1
     * dump the font file directly from the font descriptor and put the glyphs into the correct slots
     *
     * for 8bit + nonTrueType
     * re-encoding the font using a PostScript encoding list (glyph id <-> glpyh name)
     *
     * for 8bit + TrueType
     * sort the glpyhs as the original order, and later will map GID (instead of char code) to Unicode
     *
     * for CID + nonTrueType
     * Flatten the font 
     *
     * for CID Truetype
     * same as 8bitTrueType, except for that we have to check 65536 charcodes
     */
    if(!font->isCIDFont())
    {
        font_8bit = dynamic_cast<Gfx8BitFont*>(font);
        maxcode = 0xff;
        if(suffix == ".ttf")
        {
            script_fout << "Reencode(\"original\")" << endl;
            int buflen;
            char * buf = nullptr;
            if((buf = font->readEmbFontFile(xref, &buflen)))
            {
                FoFiTrueType *fftt = nullptr;
                if((fftt = FoFiTrueType::make(buf, buflen)))
                {
                    code2GID = font_8bit->getCodeToGIDMap(fftt);
                    code2GID_len = 256;
                    delete fftt;
                }
                gfree(buf);
            }
        }
        else
        {
            // move the slot such that it's consistent with the encoding seen in PDF
            ofstream out(tmp_dir / (fn + "_.encoding"));
            add_tmp_file(fn+"_.encoding");

            out << format("/%1% [") % fn << endl;
            for(int i = 0; i < 256; ++i)
            {
                auto cn = font_8bit->getCharName(i);
                out << "/" << ((cn == nullptr) ? ".notdef" : cn) << endl;
            }
            out << "] def" << endl;

            script_fout << format("LoadEncodingFile(%1%)") % (tmp_dir / (fn+"_.encoding")) << endl;
            script_fout << format("Reencode(\"%1%\")") % fn << endl;
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
    
    /*
     * Step 2
     * map charcode (or GID for CID truetype)
     * generate an Consortium encoding file and let fontforge handle it.
     *
     * - Always map to Unicode for 8bit TrueType fonts and CID fonts
     *
     * - For 8bit nonTruetype fonts:
     *   Try to calculate the correct Unicode value from the glyph names, unless param->always_apply_tounicode is set
     * 
     */

    info.use_tounicode = ((suffix == ".ttf") || (font->isCIDFont()) || (param->always_apply_tounicode));

    auto ctu = font->getToUnicode();

    ofstream map_fout(tmp_dir / (fn + ".encoding"));
    add_tmp_file(fn+".encoding");

    int cnt = 0;
    for(int i = 0; i <= maxcode; ++i)
    {
        if((suffix != ".ttf") && (font_8bit != nullptr) && (font_8bit->getCharName(i) == nullptr))
            continue;

        ++ cnt;
        map_fout << format("0x%|1$X|") % ((code2GID && (i < code2GID_len))? code2GID[i] : i);

        Unicode u, *pu=&u;

        if(info.use_tounicode)
        {
            int n = 0;
            if(ctu)
                n = ctu->mapToUnicode(i, &pu);
            u = check_unicode(pu, n, i, font);
        }
        else
        {
            u = unicode_from_font(i, font);
        }

        map_fout << format(" 0x%|1$X|") % u;
        map_fout << format(" # 0x%|1$X|") % i;

        map_fout << endl;
    }

    if(cnt > 0)
    {
        script_fout << format("LoadEncodingFile(%1%, \"%2%\")") % (tmp_dir / (fn+".encoding")) % fn << endl;
        script_fout << format("Reencode(\"%1%\", 1)") % fn << endl;
    }

    if(ctu)
        ctu->decRefCnt();

    auto dest = ((param->single_html ? tmp_dir : dest_dir) / (fn+".ttf"));
    if(param->single_html)
        add_tmp_file(fn+".ttf");

    script_fout << format("Generate(%1%)") % dest << endl;
    script_fout << "Close()" << endl;
    script_fout << format("Open(%1%, 1)") % dest << endl;

    for(const string & s1 : {"Win", "Typo", "HHead"})
    {
        for(const string & s2 : {"Ascent", "Descent"})
        {
            script_fout << "Print(GetOS2Value(\"" << s1 << s2 << "\"))" << endl;
        }
    }

    if(system((boost::format("fontforge -script %1% 1>%2% 2>%3%") % script_path % (tmp_dir / (fn+".info")) % (tmp_dir / NULL_FILENAME)).str().c_str()) != 0)
        cerr << "Warning: fontforge failed." << endl;

    add_tmp_file(fn+".info");
    add_tmp_file(NULL_FILENAME);

    // read metric
    int WinAsc, WinDes, TypoAsc, TypoDes, HHeadAsc, HHeadDes;
    if(ifstream(tmp_dir / (fn+".info")) >> WinAsc >> WinDes >> TypoAsc >> TypoDes >> HHeadAsc >> HHeadDes)
    {
        int em = TypoAsc - TypoDes;
        if(em != 0)
        {
            info.ascent = ((double)HHeadAsc) / em;
            info.descent = ((double)HHeadDes) / em;
        }
        else
        {
            info.ascent = 0;
            info.descent = 0;
        }
    }

    if(param->debug)
    {
        cerr << "Ascent: " << info.ascent << " Descent: " << info.descent << endl;
    }

    export_remote_font(info, ".ttf", "truetype", font);
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

long long HTMLRenderer::install_rise(double rise)
{
    auto iter = rise_map.lower_bound(rise - param->v_eps);
    if((iter != rise_map.end()) && (abs(iter->first - rise) < param->v_eps))
    {
        return iter->second;
    }

    long long new_rise_id = rise_map.size();
    rise_map.insert(make_pair(rise, new_rise_id));
    export_rise(new_rise_id, rise);
    return new_rise_id;
}
