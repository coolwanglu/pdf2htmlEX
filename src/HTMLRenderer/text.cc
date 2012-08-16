/*
 * text.ccc
 *
 * Handling text and relative stuffs
 *
 * by WangLu
 * 2012.08.14
 */

#include <iostream>

#include <boost/format.hpp>

#include "HTMLRenderer.h"
#include "namespace.h"

string HTMLRenderer::dump_embedded_font (GfxFont * font, long long fn_id)
{
    // mupdf consulted
    
    Object ref_obj, font_obj, font_obj2, fontdesc_obj;
    Object obj, obj1, obj2;
    Dict * dict = nullptr;

    string suffix, subtype;

    char buf[1024];
    int len;

    string fn;
    ofstream outf;

    auto * id = font->getID();
    ref_obj.initRef(id->num, id->gen);
    ref_obj.fetch(xref, &font_obj);
    ref_obj.free();

    if(!font_obj.isDict())
    {
        cerr << "Font object is not a dictionary" << endl;
        goto err;
    }

    dict = font_obj.getDict();
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
                cerr << "Unknown subtype: " << subtype << endl;
                goto err;
            }
        }
        else
        {
            cerr << "Invalid subtype in font descriptor" << endl;
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
        cerr << "Cannot find FontFile for dump" << endl;
        goto err;
    }

    if(suffix == "")
    {
        cerr << "Font type unrecognized" << endl;
        goto err;
    }

    obj.streamReset();

    fn = (format("f%|1$x|%2%")%fn_id%suffix).str();
    outf.open(tmp_dir / fn , ofstream::binary);
    add_tmp_file(fn);
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
    
    // if the line is still open, try to merge with it
    if(line_opened)
    {
        double target = (cur_tx - draw_tx) * draw_scale;
        if(target > -param->h_eps)
        {
            if(target > param->h_eps)
            {
                double w;
                auto wid = install_whitespace(target, w);
                html_fout << format("<span class=\"_ _%|1$x|\"> </span>") % wid;
                draw_tx += w / draw_scale;
            }
        }
        else
        {
            // shift left
            // TODO, create a class for this
            html_fout << format("<span style=\"margin-left:%1%px\"></span>") % target;
            draw_tx += target / draw_scale;
        }
    }

    if(!line_opened)
    {
        // have to open a new line
        
        // classes
        html_fout << "<div class=\"l "
            << format("f%|1$x| s%|2$x| c%|3$x|") % cur_fn_id % cur_fs_id % cur_color_id;
        
        // "t0" is the id_matrix
        if(cur_tm_id != 0)
            html_fout << format(" t%|1$x|") % cur_tm_id;
    
        if(cur_ls_id != 0)
            html_fout << format(" l%|1$x|") % cur_ls_id;

        if(cur_ws_id != 0)
            html_fout << format(" w%|1$x|") % cur_ws_id;


        {
            double x,y; // in user space
            state->transform(state->getCurX(), state->getCurY(), &x, &y);
            // TODO: recheck descent/ascent
            html_fout << "\" style=\""
                << "bottom:" << (y + state->getFont()->getDescent() * draw_font_size) << "px;"
                << "top:" << (pageHeight - y - state->getFont()->getAscent() * draw_font_size) << "px;"
                << "left:" << x << "px;"
                ;
        }

        //debug 
        if(0)
        {
#if 0
            html_fout << "\"";
            double x,y;
            state->transform(state->getCurX(), state->getCurY(), &x, &y);
            html_fout << format("data-lx=\"%5%\" data-ly=\"%6%\" data-drawscale=\"%4%\" data-x=\"%1%\" data-y=\"%2%\" data-hs=\"%3%")
                %x%y%(state->getHorizScaling())%draw_scale%state->getLineX()%state->getLineY();
#endif
        }

        html_fout << "\">";

        line_opened = true;
    }

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

    while (len > 0) {
        auto n = font->getNextChar(p, len, &code, &u, &uLen, &dx1, &dy1, &ox, &oy);

        if(!(_equal(ox, 0) && _equal(oy, 0)))
        {
            cerr << "TODO: non-zero origins" << endl;
        }

        if(uLen == 0)
        {
            // TODO
#if 0
            CharCode c = 0;
            for(int i = 0; i < n; ++i)
            {
                c = (c<<8) | (code&0xff);
                code >>= 8;
            }
            for(int i = 0; i < n; ++i)
            {
                Unicode u = (c&0xff);
                c >>= 8;
                outputUnicodes(html_fout, &u, 1);
            }
#endif
        }
        else
        {
            outputUnicodes(html_fout, u, uLen);
        }

        dx += dx1;
        dy += dy1;

        if (n == 1 && *p == ' ') 
        {
            ++nSpaces;
        }

        ++nChars;
        p += n;
        len -= n;
    }

    dx = (dx * state->getFontSize() 
            + nChars * state->getCharSpace() 
            + nSpaces * state->getWordSpace()) * state->getHorizScaling();
    
    dy *= state->getFontSize();

    cur_tx += dx;
    cur_ty += dy;
        
    draw_tx += dx;
    draw_ty += dy;
}
