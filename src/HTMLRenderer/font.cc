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
#include <unordered_set>

#include <GlobalParams.h>
#include <fofi/FoFiTrueType.h>
#include <CharCodeToUnicode.h>

#include "Param.h"
#include "HTMLRenderer.h"
#include "util/namespace.h"
#include "util/math.h"
#include "util/misc.h"
#include "util/base64stream.h"
#include "util/ffw.h"
#include "util/path.h"
#include "util/unicode.h"
#include "util/css_const.h"

namespace pdf2htmlEX {

using std::min;
using std::unordered_set;
using std::cerr;
using std::endl;

string HTMLRenderer::dump_embedded_font (GfxFont * font, long long fn_id)
{
    Object obj, obj1, obj2;
    Object font_obj, font_obj2, fontdesc_obj;
    string suffix;
    string filepath;

    try
    {
        // mupdf consulted
        string subtype;

        auto * id = font->getID();

        Object ref_obj;
        ref_obj.initRef(id->num, id->gen);
        ref_obj.fetch(xref, &font_obj);
        ref_obj.free();

        if(!font_obj.isDict())
        {
            cerr << "Font object is not a dictionary" << endl;
            throw 0;
        }

        Dict * dict = font_obj.getDict();
        if(dict->lookup("DescendantFonts", &font_obj2)->isArray())
        {
            if(font_obj2.arrayGetLength() == 0)
            {
                cerr << "Warning: empty DescendantFonts array" << endl;
            }
            else
            {
                if(font_obj2.arrayGetLength() > 1)
                    cerr << "TODO: multiple entries in DescendantFonts array" << endl;

                if(font_obj2.arrayGet(0, &obj2)->isDict())
                {
                    dict = obj2.getDict();
                }
            }
        }

        if(!dict->lookup("FontDescriptor", &fontdesc_obj)->isDict())
        {
            cerr << "Cannot find FontDescriptor " << endl;
            throw 0;
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
                    cerr << "Unknown subtype: " << subtype << endl;
                    throw 0;
                }
            }
            else
            {
                cerr << "Invalid subtype in font descriptor" << endl;
                throw 0;
            }
        }
        else if (dict->lookup("FontFile2", &obj)->isStream())
        { 
            suffix = ".ttf";
        }
        else if (dict->lookup("FontFile", &obj)->isStream())
        {
            suffix = ".pfa";
        }
        else
        {
            cerr << "Cannot find FontFile for dump" << endl;
            throw 0;
        }

        if(suffix == "")
        {
            cerr << "Font type unrecognized" << endl;
            throw 0;
        }

        obj.streamReset();

        filepath = (char*)str_fmt("%s/f%llx%s", param->tmp_dir.c_str(), fn_id, suffix.c_str());
        tmp_files.add(filepath);

        ofstream outf(filepath, ofstream::binary);
        if(!outf)
            throw string("Cannot open file ") + filepath + " for writing";

        char buf[1024];
        int len;
        while((len = obj.streamGetChars(1024, (Guchar*)buf)) > 0)
        {
            outf.write(buf, len);
        }
        outf.close();
        obj.streamClose();
    }
    catch(int) 
    {
        cerr << "Someting wrong when trying to dump font " << hex << fn_id << dec << endl;
    }

    obj2.free();
    obj1.free();
    obj.free();

    fontdesc_obj.free();
    font_obj2.free();
    font_obj.free();

    return filepath;
}

void HTMLRenderer::embed_font(const string & filepath, GfxFont * font, FontInfo & info, bool get_metric_only)
{
    if(param->debug)
    {
        cerr << "Embed font: " << filepath << " " << info.id << endl;
    }

    ffw_load_font(filepath.c_str());
    ffw_prepare_font();

    if(param->debug)
    {
        auto fn = str_fmt("%s/__raw_font_%llx", param->tmp_dir.c_str(), info.id, param->font_suffix.c_str());
        tmp_files.add((char*)fn);
        ofstream((char*)fn, ofstream::binary) << ifstream(filepath).rdbuf();
    }

    int * code2GID = nullptr;
    int code2GID_len = 0;
    int maxcode = 0;

    Gfx8BitFont * font_8bit = nullptr;
    GfxCIDFont * font_cid = nullptr;

    string suffix = get_suffix(filepath);
    for(auto iter = suffix.begin(); iter != suffix.end(); ++iter)
        *iter = tolower(*iter);

    /*
     * if parm->tounicode is 0, try the provided tounicode map first
     */
    info.use_tounicode = (is_truetype_suffix(suffix) || (param->tounicode >= 0));
    bool has_space = false;

    const char * used_map = nullptr;

    info.em_size = ffw_get_em_size();
    info.space_width = 0;

    if(!font->isCIDFont())
    {
        font_8bit = dynamic_cast<Gfx8BitFont*>(font);
    }
    else
    {
        font_cid = dynamic_cast<GfxCIDFont*>(font);
    }

    if(get_metric_only)
    {
        ffw_metric(&info.ascent, &info.descent);
        ffw_close();
        return;
    }

    used_map = preprocessor.get_code_map(hash_ref(font->getID()));

    /*
     * Step 1
     * dump the font file directly from the font descriptor and put the glyphs into the correct slots *
     * for 8bit + nonTrueType
     * re-encoding the font by glyph names
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
    if(font_8bit)
    {
        maxcode = 0xff;
        if(is_truetype_suffix(suffix))
        {
            ffw_reencode_glyph_order();
            FoFiTrueType *fftt = nullptr;
            if((fftt = FoFiTrueType::load((char*)filepath.c_str())) != nullptr)
            {
                code2GID = font_8bit->getCodeToGIDMap(fftt);
                code2GID_len = 256;
                delete fftt;
            }
        }
        else
        {
            // move the slot such that it's consistent with the encoding seen in PDF
            unordered_set<string> nameset;
            bool name_conflict_warned = false;

            memset(cur_mapping2, 0, 0x100 * sizeof(char*));

            for(int i = 0; i < 256; ++i)
            {
                if(!used_map[i]) continue;

                auto cn = font_8bit->getCharName(i);
                if(cn == nullptr)
                {
                    continue;
                }
                else
                {
                    if(nameset.insert(string(cn)).second)
                    {
                        cur_mapping2[i] = cn;    
                    }
                    else
                    {
                        if(!name_conflict_warned)
                        {
                            name_conflict_warned = true;
                            //TODO: may be resolved using advanced font properties?
                            cerr << "Warning: encoding confliction detected in font: " << hex << info.id << dec << endl;
                        }
                    }
                }
            }

            ffw_reencode_raw2(cur_mapping2, 256, 0);
        }
    }
    else
    {
        maxcode = 0xffff;

        if(is_truetype_suffix(suffix))
        {
            ffw_reencode_glyph_order();

            GfxCIDFont * _font = dynamic_cast<GfxCIDFont*>(font);

            // code2GID has been stored for embedded CID fonts
            code2GID = _font->getCIDToGID();
            code2GID_len = _font->getCIDToGIDLen();
        }
        else
        {
            ffw_cidflatten();
        }
    }

    /*
     * Step 2
     * - map charcode (or GID for CID truetype)
     *
     * -> Always map to Unicode for 8bit TrueType fonts and CID fonts
     *
     * -> For 8bit nonTruetype fonts:
     *   Try to calculate the correct Unicode value from the glyph names, when collision is detected in ToUnicode Map
     * 
     * - Fill in the width_list, and set widths accordingly
     * - Remove unused glyphs
     */


    {
        unordered_set<int> codeset;
        bool name_conflict_warned = false;

        auto ctu = font->getToUnicode();
        memset(cur_mapping, -1, 0x10000 * sizeof(*cur_mapping));
        memset(width_list, -1, 0x10000 * sizeof(*width_list));

        if(code2GID)
            maxcode = min<int>(maxcode, code2GID_len - 1);

        bool is_truetype = is_truetype_suffix(suffix);
        int max_key = maxcode;
        /*
         * Traverse all possible codes
         */
        bool retried = false; // avoid infinite loop
        for(int cur_code = 0; cur_code <= maxcode; ++cur_code)
        {
            if(!used_map[cur_code])
                continue;

            /*
             * Skip glyphs without names (only for non-ttf fonts)
             */
            if(!is_truetype && (font_8bit != nullptr) 
                    && (font_8bit->getCharName(cur_code) == nullptr))
            {
                continue;
            }

            int mapped_code = cur_code;
            if(code2GID)
            {
                // for fonts with GID (e.g. TTF) we need to map GIDs instead of codes
                if((mapped_code = code2GID[cur_code]) == 0) continue;
            }

            if(mapped_code > max_key)
                max_key = mapped_code;

            Unicode u, *pu=&u;
            if(info.use_tounicode)
            {
                int n = ctu ? (ctu->mapToUnicode(cur_code, &pu)) : 0;
                u = check_unicode(pu, n, cur_code, font);
            }
            else
            {
                u = unicode_from_font(cur_code, font);
            }

            if(codeset.insert(u).second)
            {
                cur_mapping[mapped_code] = u;
            }
            else
            {
                // collision detected
                if(param->tounicode == 0)
                {
                    // in auto mode, just drop the tounicode map
                    if(!retried)
                    {
                        cerr << "ToUnicode CMap is not valid and got dropped for font: " << hex << info.id << dec << endl;
                        retried = true;
                        codeset.clear();
                        info.use_tounicode = false;
                        //TODO: constant for the length
                        memset(cur_mapping, -1, 0x10000 * sizeof(*cur_mapping));
                        memset(width_list, -1, 0x10000 * sizeof(*width_list));
                        cur_code = -1;
                        continue;
                    }
                }
                if(!name_conflict_warned)
                {
                    name_conflict_warned = true;
                    //TODO: may be resolved using advanced font properties?
                    cerr << "Warning: encoding confliction detected in font: " << hex << info.id << dec << endl;
                }
            }

            {
                double cur_width = 0;
                if(font_8bit)
                {
                    cur_width = font_8bit->getWidth(cur_code);
                }
                else
                {
                    char buf[2];  
                    buf[0] = (cur_code >> 8) & 0xff;
                    buf[1] = (cur_code & 0xff);
                    cur_width = font_cid->getWidth(buf, 2) ;
                }
                width_list[mapped_code] = (int)floor(cur_width * info.em_size + 0.5);

                if(u == ' ')
                {
                    has_space = true;
                    info.space_width = cur_width;
                }
            }
        }

        ffw_set_widths(width_list, max_key + 1, param->stretch_narrow_glyph, param->squeeze_wide_glyph, param->remove_unused_glyph);
        
        ffw_reencode_raw(cur_mapping, max_key + 1, 1);

        // In some space offsets in HTML, we insert a ' ' there in order to improve text copy&paste
        // We need to make sure that ' ' is in the font, otherwise it would be very ugly if you select the text
        // Might be a problem if ' ' is in the font, but not empty
        if(!has_space)
        {
            if(font_8bit)
            {
                info.space_width = font_8bit->getWidth(' ');
            }
            else
            {
                char buf[2] = {0, ' '};
                info.space_width = font_cid->getWidth(buf, 2);
            }
            ffw_add_empty_char((int32_t)' ', (int)floor(info.space_width * info.em_size + 0.5));
        }

        if(ctu)
            ctu->decRefCnt();
    }

    /*
     * Step 3
     *
     * Generate the font as desired
     *
     */
    string cur_tmp_fn = (char*)str_fmt("%s/__tmp_font1%s", param->tmp_dir.c_str(), param->font_suffix.c_str());
    tmp_files.add(cur_tmp_fn);
    string other_tmp_fn = (char*)str_fmt("%s/__tmp_font2%s", param->tmp_dir.c_str(), param->font_suffix.c_str());
    tmp_files.add(other_tmp_fn);

    ffw_save(cur_tmp_fn.c_str());

    ffw_close();

    /*
     * Step 4
     * Font Hinting
     */
    bool hinted = false;

    // Call external hinting program if specified 
    if(param->external_hint_tool != "")
    {
        hinted = (system((char*)str_fmt("%s \"%s\" \"%s\"", param->external_hint_tool.c_str(), cur_tmp_fn.c_str(), other_tmp_fn.c_str())) == 0);
    }

    // Call internal hinting procedure if specified 
    if((!hinted) && (param->auto_hint))
    {
        ffw_load_font(cur_tmp_fn.c_str());
        ffw_auto_hint();
        ffw_save(other_tmp_fn.c_str());
        ffw_close();
        hinted = true;
    }

    if(hinted)
    {
        swap(cur_tmp_fn, other_tmp_fn);
    }

    /* 
     * Step 5 
     * Generate the font and load the metrics
     *
     * Ascent/Descent are not used in PDF, and the values in PDF may be wrong or inconsistent (there are 3 sets of them)
     * We need to reload in order to retrieve/fix accurate ascent/descent, some info won't be written to the font by fontforge until saved.
     */
    string fn = (char*)str_fmt("%s/f%llx%s", 
        (param->single_html ? param->tmp_dir : param->dest_dir).c_str(),
        info.id, param->font_suffix.c_str());

    if(param->single_html)
        tmp_files.add(fn);

    ffw_load_font(cur_tmp_fn.c_str());
    ffw_metric(&info.ascent, &info.descent);
    ffw_save(fn.c_str());

    ffw_close();
}


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
        new_font_info.em_size = 0;
        new_font_info.space_width = 0;
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
             << "font-family:" << CSS::FONT_FAMILY_CN << info.id << ";"
             << "src:url(";

    {
        auto fn = str_fmt("f%llx%s", info.id, suffix.c_str());
        if(param->single_html)
        {
            auto path = param->tmp_dir + "/" + (char*)fn;
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw "Cannot locate font file: " + path;
            f_css.fs << "'data:" + mime_type + ";base64," << base64stream(fin) << "'";
        }
        else
        {
            f_css.fs << (char*)fn;
        }
    }

    f_css.fs << ")"
             << "format(\"" << format << "\");"
             << "}" // end of @font-face
             << "." << CSS::FONT_FAMILY_CN << info.id << "{"
             << "font-family:" << CSS::FONT_FAMILY_CN << info.id << ";"
             << "line-height:" << round(info.ascent - info.descent) << ";"
             << "font-style:normal;"
             << "font-weight:normal;"
             << "visibility:visible;"
             << "}" 
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
    f_css.fs << "." << CSS::FONT_FAMILY_CN << fn_id << "{font-family:sans-serif;visibility:hidden;}" << endl;
}

void HTMLRenderer::export_local_font(const FontInfo & info, GfxFont * font, const string & original_font_name, const string & cssfont) 
{
    f_css.fs << "." << CSS::FONT_FAMILY_CN << info.id << "{";
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

} //namespace pdf2htmlEX
