/*
 * text.cc
 *
 * Handling text & font, and relative stuffs
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */

#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <cctype>
#include <cmath>

#include <CharCodeToUnicode.h>
#include <fofi/FoFiTrueType.h>

#include "HTMLRenderer.h"
#include "TextLineBuffer.h"
#include "util/ffw.h"
#include "util/namespace.h"
#include "util/unicode.h"
#include "util/path.h"
#include "util/math.h"
#include "util/misc.h"

namespace pdf2htmlEX {

using std::unordered_set;
using std::min;
using std::all_of;
using std::floor;
using std::swap;
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
        auto fn = str_fmt("%s/__raw_font_%lld", param->tmp_dir.c_str(), info.id, param->font_suffix.c_str());
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
    if(!font->isCIDFont())
    {
        font_8bit = dynamic_cast<Gfx8BitFont*>(font);
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
        font_cid = dynamic_cast<GfxCIDFont*>(font);
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
        for(int i = 0; i <= maxcode; ++i)
        {
            if(!used_map[i])
                continue;

            /*
             * Skip glyphs without names (only for non-ttf fonts)
             */
            if(!is_truetype && (font_8bit != nullptr) 
                    && (font_8bit->getCharName(i) == nullptr))
            {
                continue;
            }

            int k = i;
            if(code2GID)
            {
                if((k = code2GID[i]) == 0) continue;
            }

            if(k > max_key)
                max_key = k;

            Unicode u, *pu=&u;
            if(info.use_tounicode)
            {
                int n = ctu ? (ctu->mapToUnicode(i, &pu)) : 0;
                u = check_unicode(pu, n, i, font);
            }
            else
            {
                u = unicode_from_font(i, font);
            }

            if(u == ' ')
                has_space = true;

            if(codeset.insert(u).second)
            {
                cur_mapping[k] = u;
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
                        i = -1;
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

            if(font_8bit)
            {
                width_list[k] = (int)floor(font_8bit->getWidth(i) * info.em_size + 0.5);
            }
            else
            {
                char buf[2];  
                buf[0] = (i >> 8) & 0xff;
                buf[1] = (i & 0xff);
                width_list[k] = (int)floor(font_cid->getWidth(buf, 2) * info.em_size + 0.5);
            }
        }

        ffw_set_widths(width_list, max_key + 1, param->stretch_narrow_glyph, param->squeeze_wide_glyph, param->remove_unused_glyph);
        
        ffw_reencode_raw(cur_mapping, max_key + 1, 1);

        // we need the space character for offsets
        if(!has_space)
        {
            int space_width;
            if(font_8bit)
            {
                space_width = (int)floor(font_8bit->getWidth(' ') * info.em_size + 0.5);
            }
            else
            {
                char buf[2] = {0, ' '};
                space_width = (int)floor(font_cid->getWidth(buf, 2) * info.em_size + 0.5);
            }
            ffw_make_char((int)' ', space_width);
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
     * Generate the font
     * Reload to retrieve/fix accurate ascent/descent
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

void HTMLRenderer::drawString(GfxState * state, GooString * s)
{
    if(s->getLength() == 0)
        return;

    auto font = state->getFont();
    double cur_letter_space = state->getCharSpace();

    // Writing mode fonts and Type 3 fonts are rendered as images
    // I don't find a way to display writing mode fonts in HTML except for one div for each character, which is too costly
    // For type 3 fonts, due to the font matrix, still it's hard to show it on HTML
    if( (font == nullptr) 
        || (font->getWMode())
        || (font->getType() == fontType3)
      )
    {
        return;
    }

    //hidden
    if((state->getRender() & 3) == 3)
    {
        return;
    }

    // see if the line has to be closed due to state change
    check_state_change(state);
    prepare_text_line(state);

    // Now ready to output
    // get the unicodes
    char *p = s->getCString();
    int len = s->getLength();

    double dx = 0;
    double dy = 0;
    double dx1,dy1;
    double ox, oy;

    int nChars = 0;
    int nSpaces = 0;
    int uLen;

    CharCode code;
    Unicode *u = nullptr;

    while (len > 0) 
    {
        auto n = font->getNextChar(p, len, &code, &u, &uLen, &dx1, &dy1, &ox, &oy);

        if(!(equal(ox, 0) && equal(oy, 0)))
        {
            cerr << "TODO: non-zero origins" << endl;
        }

        bool is_space = false;
        if (n == 1 && *p == ' ') 
        {
            ++nSpaces;
            is_space = true;
        }
        
        if(is_space && (param->space_as_offset))
        {
            // ignore horiz_scaling, as it's merged in CTM
            text_line_buf->append_offset((dx1 * cur_font_size + cur_letter_space + cur_word_space) * draw_text_scale); 
        }
        else
        {
            if((param->decompose_ligature) && (uLen > 1) && all_of(u, u+uLen, isLegalUnicode))
            {
                text_line_buf->append_unicodes(u, uLen);
            }
            else
            {
                if(cur_font_info->use_tounicode)
                {
                    Unicode uu = check_unicode(u, uLen, code, font);
                    text_line_buf->append_unicodes(&uu, 1);
                }
                else
                {
                    Unicode uu = unicode_from_font(code, font);
                    text_line_buf->append_unicodes(&uu, 1);
                }
            }
        }

        dx += dx1;
        dy += dy1;

        ++nChars;
        p += n;
        len -= n;
    }

    double hs = state->getHorizScaling();

    // horiz_scaling is merged into ctm now, 
    // so the coordinate system is ugly
    dx = (dx * cur_font_size + nChars * cur_letter_space + nSpaces * cur_word_space) * hs;
    
    dy *= cur_font_size;

    cur_tx += dx;
    cur_ty += dy;
        
    draw_tx += dx;
    draw_ty += dy;
}

} // namespace pdf2htmlEX
