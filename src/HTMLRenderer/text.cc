/*
 * text.ccc
 *
 * Handling text and relative stuffs
 *
 * by WangLu
 * 2012.08.14
 */

#include <iostream>
#include <algorithm>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <CharCodeToUnicode.h>
#include <fofi/FoFiTrueType.h>

#include "HTMLRenderer.h"
#include "namespace.h"
#include "config.h"

using boost::algorithm::to_lower;

path HTMLRenderer::dump_embedded_font (GfxFont * font, long long fn_id)
{
    Object obj, obj1, obj2;
    Object font_obj, font_obj2, fontdesc_obj;
    string suffix;
    path filepath;

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

        string fn = (format("f%|1$x|")%fn_id).str();
        ofstream outf;
        filepath = tmp_dir / (fn + suffix);
        outf.open(filepath, ofstream::binary);
        add_tmp_file(fn+suffix);

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
        cerr << format("Someting wrong when trying to dump font %|1$x|") % fn_id << endl;
    }

    obj2.free();
    obj1.free();
    obj.free();

    fontdesc_obj.free();
    font_obj2.free();
    font_obj.free();

    return filepath;
}

void HTMLRenderer::embed_font(const path & filepath, GfxFont * font, FontInfo & info)
{
    string suffix = filepath.extension().string();
    to_lower(suffix);

    string fn = (format("f%|1$x|") % info.id).str();

    path script_path = tmp_dir / (fn + ".pe");
    ofstream script_fout(script_path, ofstream::binary);
    add_tmp_file(fn+".pe");

    script_fout << format("Open(%1%, 1)") % filepath << endl;

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
        if((suffix == ".ttf") || (suffix == ".ttc") || (suffix == ".otf"))
        {
            script_fout << "Reencode(\"original\")" << endl;
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

    auto dest = ((param->single_html ? tmp_dir : dest_dir) / (fn+(param->font_suffix)));
    if(param->single_html)
        add_tmp_file(fn+(param->font_suffix));

    /*
     * [Win|Typo|HHead][Ascent|Descent]
     * Firefox & Chrome interprets the values in different ways
     * Trying to unify them 
     */
    script_fout << format("Generate(%1%)") % dest << endl;
    script_fout << "Close()" << endl;
    script_fout << format("Open(%1%, 1)") % dest << endl;
    script_fout << ifstream(PDF2HTMLEX_DATA_PATH / UNIFY_SCRIPT_FILENAME).rdbuf();
    script_fout << format("Generate(%1%)") % dest << endl;
    script_fout.close();

    if(system((boost::format("fontforge -script %1% 1>%2% 2>%3%") % script_path % (tmp_dir / (fn+".info")) % (tmp_dir / NULL_FILENAME)).str().c_str()) != 0)
        cerr << "Warning: fontforge failed." << endl;

    add_tmp_file(fn+".info");
    add_tmp_file(NULL_FILENAME);

    // read metric
    int em, ascent, descent;
    if(ifstream(tmp_dir / (fn+".info")) >> em >> ascent >> descent)
    {
        if(em != 0)
        {
            info.ascent = ((double)ascent) / em;
            info.descent = -((double)descent) / em;
        }
        else
        {
            info.ascent = 0;
            info.descent = 0;
        }
    }
    else
    {
        cerr << "Warning: cannot read font info for " << fn << endl;
        info.ascent = font->getAscent();
        info.descent = font->getDescent();
    }

    if(param->debug)
    {
        cerr << "Ascent: " << info.ascent << " Descent: " << info.descent << endl;
    }

    export_remote_font(info, param->font_suffix, param->font_format, font);
}

void HTMLRenderer::drawString(GfxState * state, GooString * s)
{
    if(s->getLength() == 0)
        return;

    auto font = state->getFont();
    if((font == nullptr) || (font->getWMode()))
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
    prepare_line(state);

    // Now ready to output
    // get the unicodes
    char *p = s->getCString();
    int len = s->getLength();

    double dx = 0;
    double dy = 0;
    double dxerr = 0;
    double dx1,dy1;
    double ox, oy;

    int nChars = 0;
    int nSpaces = 0;
    int uLen;

    CharCode code;
    Unicode *u = nullptr;

    double fs = state->getFontSize();
    double cs = state->getCharSpace();
    double ws = state->getWordSpace();
    double hs = state->getHorizScaling();

    while (len > 0) {
        auto n = font->getNextChar(p, len, &code, &u, &uLen, &dx1, &dy1, &ox, &oy);
        
        if(!(_equal(ox, 0) && _equal(oy, 0)))
        {
            cerr << "TODO: non-zero origins" << endl;
        }

        if (n == 1 && *p == ' ') 
        {
            ++nSpaces;
        }
        
        Unicode uu = (cur_font_info.use_tounicode ? check_unicode(u, uLen, code, font) : unicode_from_font(code, font));
        outputUnicodes(line_buf, &uu, 1);

        dx += dx1;
        dy += dy1;

        ++nChars;
        p += n;
        len -= n;
    }

    // horiz_scaling is merged into ctm now, 
    // so the coordinate system is ugly
    dx = (dx * fs + nChars * cs + nSpaces * ws) * hs;
    
    dy *= fs;

    cur_tx += dx;
    cur_ty += dy;
        
    draw_tx += dx + dxerr * state->getFontSize() * state->getHorizScaling();
    draw_ty += dy;
}
