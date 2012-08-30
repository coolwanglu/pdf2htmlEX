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

#include <fofi/FoFiType1C.h>

#include "HTMLRenderer.h"
#include "namespace.h"

using std::all_of;

string HTMLRenderer::dump_embedded_font (GfxFont * font, long long fn_id)
{
    Object obj, obj1, obj2;
    Object font_obj, font_obj2, fontdesc_obj;
    string suffix;

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
        outf.open(tmp_dir / (fn + suffix), ofstream::binary);
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

    return suffix;
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
