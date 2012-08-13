/*
 * HTMLRenderer.cc
 *
 * Copyright (C) 2011 by Hongliang TIAN(tatetian@gmail.com)
 * Copyright (C) 2012 by Lu Wang coolwanglu<at>gmail.com
 */

/*
 * TODO
 * font base64 embedding
 */

#include <cmath>
#include <cassert>
#include <fstream>
#include <algorithm>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
// for gil bug
const int *int_p_NULL = nullptr;
#include <boost/gil/gil_all.hpp>
#include <boost/gil/extension/io/png_dynamic_io.hpp>

#include <GfxFont.h>
#include <CharCodeToUnicode.h>
#include <fofi/FoFiType1C.h>
#include <fofi/FoFiTrueType.h>
#include <splash/SplashBitmap.h>

#include "HTMLRenderer.h"
#include "BackgroundRenderer.h"
#include "Consts.h"
#include "util.h"

/*
 * CSS classes
 *
 * p - Page
 * l - Line
 * w - White space
 * i - Image
 *
 * 
 * Reusable CSS classes
 *
 * f<hex> - Font (also for font names)
 * s<hex> - font Size
 * w<hex> - White space
 * t<hex> - Transform matrix
 * c<hex> - Color
 *
 */

HTMLRenderer::HTMLRenderer(const Param * param)
    :line_opened(false)
    ,html_fout(param->output_filename.c_str(), ofstream::binary)
    ,allcss_fout("all.css")
    ,fontscript_fout(TMP_DIR+"/convert.pe")
    ,image_count(0)
    ,param(param)
{
    // install default font & size
    install_font(nullptr);
    install_font_size(0);

    install_transform_matrix(id_matrix);

    GfxRGB black;
    black.r = black.g = black.b = 0;
    install_color(&black);
    
    html_fout << HTML_HEAD << endl;
}

HTMLRenderer::~HTMLRenderer()
{
    html_fout << HTML_TAIL << endl;
}

void HTMLRenderer::process(PDFDoc *doc)
{
    std::cerr << "Processing Text: ";

    xref = doc->getXRef();
    for(int i = param->first_page; i <= param->last_page ; ++i) 
    {
        doc->displayPage(this, i, param->h_dpi, param->v_dpi,
                0, true, false, false,
                nullptr, nullptr, nullptr, nullptr);

        std::cerr << ".";
        std::cerr.flush();
    }
    std::cerr << std::endl;

    if(param->process_nontext)
    {
        // Render non-text objects as image
        std::cerr << "Processing Others: ";
        // copied from poppler
        SplashColor color;
        color[0] = color[1] = color[2] = 255;

        auto bg_renderer = new BackgroundRenderer(splashModeRGB8, 4, gFalse, color);
        bg_renderer->startDoc(doc);

        for(int i = param->first_page; i <= param->last_page ; ++i) 
        {
            doc->displayPage(bg_renderer, i, param->h_dpi2, param->v_dpi2,
                    0, true, false, false,
                    nullptr, nullptr, nullptr, nullptr);
            bg_renderer->getBitmap()->writeImgFile(splashFormatPng, (char*)(boost::format("p%|1$x|.png")%i).str().c_str(), param->h_dpi2, param->v_dpi2);

            std::cerr << ".";
            std::cerr.flush();
        }
        delete bg_renderer;
        std::cerr << std::endl;
    }
}

void HTMLRenderer::startPage(int pageNum, GfxState *state) 
{
    this->pageNum = pageNum;
    this->pageWidth = state->getPageWidth();
    this->pageHeight = state->getPageHeight();

    assert(!line_opened);

    html_fout << boost::format("<div id=\"page-%3%\" class=\"p\" style=\"width:%1%px;height:%2%px;") % pageWidth % pageHeight % pageNum;

    html_fout << boost::format("background-image:url(p%|3$x|.png);background-position:0 0;background-size:%1%px %2%px;background-repeat:no-repeat;") % pageWidth % pageHeight % pageNum;
            
    html_fout << "\">" << endl;

    cur_fn_id = cur_fs_id = cur_tm_id = cur_color_id = 0;
    cur_tx = cur_ty = 0;
    cur_font_size = 0;

    memcpy(cur_ctm, id_matrix, sizeof(cur_ctm));
    memcpy(draw_ctm, id_matrix, sizeof(draw_ctm));
    draw_font_size = 0;
    draw_scale = 1.0;
    draw_tx = draw_ty = 0;

    cur_color.r = cur_color.g = cur_color.b = 0;
    
    reset_state_track();
}

void HTMLRenderer::endPage() {
    close_cur_line();
    // close page
    html_fout << "</div>" << endl;
}

void HTMLRenderer::close_cur_line()
{
    if(line_opened)
    {
        html_fout << "</div>" << endl;
        line_opened = false;
    }
}

void HTMLRenderer::updateAll(GfxState * state) 
{ 
    all_changed = true; 
    updateTextPos(state);
}

void HTMLRenderer::updateFont(GfxState * state) 
{
    font_changed = true; 
}

void HTMLRenderer::updateTextMat(GfxState * state) 
{
    text_mat_changed = true; 
}

void HTMLRenderer::updateCTM(GfxState * state, double m11, double m12, double m21, double m22, double m31, double m32) 
{
    ctm_changed = true; 
}

void HTMLRenderer::updateTextPos(GfxState * state) 
{
    text_pos_changed = true;
    cur_tx = state->getLineX(); 
    cur_ty = state->getLineY(); 
}

void HTMLRenderer::updateTextShift(GfxState * state, double shift) 
{
    text_pos_changed = true;
    cur_tx -= shift * 0.001 * state->getFontSize() * state->getHorizScaling(); 
}

void HTMLRenderer::updateFillColor(GfxState * state) 
{
    color_changed = true; 
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
                html_fout << boost::format("<span class=\"w w%|1$x|\"> </span>") % wid;
                draw_tx += w / draw_scale;
            }
        }
        else
        {
            // or can we shift left using simple tags?
            close_cur_line();
        }
    }

    if(!line_opened)
    {
        // have to open a new line
        
        // classes
        html_fout << "<div class=\"l "
            << boost::format("f%|1$x| s%|2$x| c%|3$x|") % cur_fn_id % cur_fs_id % cur_color_id;
        
        // "t0" is the id_matrix
        if(cur_tm_id != 0)
            html_fout << boost::format(" t%|1$x|") % cur_tm_id;

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
        
        // TODO: tracking
        // letter & word spacing
        if(_is_positive(state->getCharSpace()))
            html_fout << "letter-spacing:" << state->getCharSpace() << "px;";
        if(_is_positive(state->getWordSpace()))
            html_fout << "word-spacing:" << state->getWordSpace() << "px;";

        //debug 
        //real pos & hori_scale
        if(0)
        {
#if 0
            html_fout << "\"";
            double x,y;
            state->transform(state->getCurX(), state->getCurY(), &x, &y);
            html_fout << boost::format("data-lx=\"%5%\" data-ly=\"%6%\" data-drawscale=\"%4%\" data-x=\"%1%\" data-y=\"%2%\" data-hs=\"%3%")
                %x%y%(state->getHorizScaling())%draw_scale%state->getLineX()%state->getLineY();
#endif
        }

        html_fout << "\">";

        line_opened = true;

        draw_tx = cur_tx;
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
            std::cerr << "TODO: non-zero origins" << std::endl;
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

void HTMLRenderer::drawImage(GfxState * state, Object * ref, Stream * str, int width, int height, GfxImageColorMap * colorMap, GBool interpolate, int *maskColors, GBool inlineImg)
{
    if(maskColors)
        return;

    boost::gil::rgb8_image_t img(width, height);
    auto imgview = view(img);
    auto loc = imgview.xy_at(0,0);

    ImageStream * img_stream = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
    img_stream->reset();

    for(int i = 0; i < height; ++i)
    {
        auto p = img_stream->getLine();
        for(int j = 0; j < width; ++j)
        {
            GfxRGB rgb;
            colorMap->getRGB(p, &rgb);

            *loc = boost::gil::rgb8_pixel_t(colToByte(rgb.r), colToByte(rgb.g), colToByte(rgb.b));

            p += colorMap->getNumPixelComps();

            ++ loc.x();
        }

        loc = imgview.xy_at(0, i+1);
    }

    boost::gil::png_write_view((boost::format("i%|1$x|.png")%image_count).str(), imgview);
    
    img_stream->close();
    delete img_stream;

    close_cur_line();

    double * ctm = state->getCTM();
    ctm[4] = ctm[5] = 0.0;
    html_fout << boost::format("<img class=\"i t%2%\" style=\"left:%3%px;bottom:%4%px;width:%5%px;height:%6%px;\" src=\"i%|1$x|.png\" />") % image_count % install_transform_matrix(ctm) % state->getCurX() % state->getCurY() % width % height << endl;


    ++ image_count;
}

// The font installation code is stolen from PSOutputDev.cc in poppler

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

void HTMLRenderer::export_remote_font(long long fn_id, const string & suffix, const string & format, GfxFont * font)
{
    allcss_fout << boost::format("@font-face{font-family:f%|1$x|;src:url(f%|1$x|%2%)format(\"%3%\");}.f%|1$x|{font-family:f%|1$x|;") % fn_id % suffix % format;

    double a = font->getAscent();
    double d = font->getDescent();
    double r = _is_positive(a-d) ? (a/(a-d)) : 1.0;

    for(const std::string & prefix : {"", "-ms-", "-moz-", "-webkit-", "-o-"})
    {
        allcss_fout << prefix << "transform-origin:0% " << (r*100.0) << "%;";
    }

    allcss_fout << "line-height:" << (a-d) << ";";

    allcss_fout << "}" << endl;
}

// TODO: this function is called when some font is unable to process, may use the name there as a hint
void HTMLRenderer::export_remote_default_font(long long fn_id)
{
    allcss_fout << boost::format(".f%|1$x|{font-family:sans-serif;color:transparent;visibility:hidden;}")%fn_id << endl;
}

void HTMLRenderer::export_local_font(long long fn_id, GfxFont * font, const string & original_font_name, const string & cssfont)
{
    allcss_fout << boost::format(".f%|1$x|{") % fn_id;
    allcss_fout << "font-family:" << ((cssfont == "") ? (original_font_name + "," + general_font_family(font)) : cssfont) << ";";

    if(font->isBold())
        allcss_fout << "font-weight:bold;";

    if(boost::algorithm::ifind_first(original_font_name, "oblique"))
        allcss_fout << "font-style:oblique;";
    else if(font->isItalic())
        allcss_fout << "font-style:italic;";

    double a = font->getAscent();
    double d = font->getDescent();
    double r = _is_positive(a-d) ? (a/(a-d)) : 1.0;

    for(const std::string & prefix : {"", "-ms-", "-moz-", "-webkit-", "-o-"})
    {
        allcss_fout << prefix << "transform-origin:0% " << (r*100.0) << "%;";
    }

    allcss_fout << "line-height:" << (a-d) << ";";

    allcss_fout << "}" << endl;
}

std::string HTMLRenderer::general_font_family(GfxFont * font)
{
    if(font -> isFixedWidth())
        return "monospace";
    else if (font -> isSerif())
        return "serif";
    else
        return "sans-serif";
}

void HTMLRenderer::export_font_size (long long fs_id, double font_size)
{
    allcss_fout << boost::format(".s%|1$x|{font-size:%2%px;}") % fs_id % font_size << endl;
}

void HTMLRenderer::export_whitespace (long long ws_id, double ws_width)
{
    allcss_fout << boost::format(".w%|1$x|{width:%2%px;}") % ws_id % ws_width << endl;
}

void HTMLRenderer::export_transform_matrix (long long tm_id, const double * tm)
{
    allcss_fout << boost::format(".t%|1$x|{") % tm_id;

    // TODO: recognize common matices
    if(_tm_equal(tm, id_matrix))
    {
        // no need to output anything
    }
    else
    {
        for(const std::string & prefix : {"", "-ms-", "-moz-", "-webkit-", "-o-"})
        {
            // PDF use a different coordinate system from Web
            allcss_fout << prefix << "transform:matrix("
                << tm[0] << ','
                << -tm[1] << ','
                << -tm[2] << ','
                << tm[3] << ',';

            if(prefix == "-moz-")
                allcss_fout << boost::format("%1%px,%2%px);") % tm[4] % -tm[5];
            else
                allcss_fout << boost::format("%1%,%2%);") % tm[4] % -tm[5];
        }
    }
    allcss_fout << "}" << endl;
}

void HTMLRenderer::export_color(long long color_id, const GfxRGB * rgb)
{
    allcss_fout << boost::format(".c%|1$x|{color:rgb(%2%,%3%,%4%);}") 
        % color_id % (int)colToByte(rgb->r) % (int)colToByte(rgb->g) % (int)colToByte(rgb->b)
        << endl;
}

void HTMLRenderer::check_state_change(GfxState * state)
{
    bool close_line = false;

    if(all_changed || text_pos_changed)
    {
        if(!(std::abs(cur_ty - draw_ty) * draw_scale < param->v_eps))
        {
            close_line = true;
            draw_ty = cur_ty;
            draw_tx = cur_tx;
        }
    }

    // TODO, we may use nested span if only color has been changed
    if(all_changed || color_changed)
    {
        GfxRGB new_color;
        state->getFillRGB(&new_color);
        if(!((new_color.r == cur_color.r) && (new_color.g == cur_color.g) && (new_color.b == cur_color.b)))
        {
            close_line = true;
            cur_color = new_color;
            cur_color_id = install_color(&new_color);
        }
    }

    bool need_rescale_font = false;
    if(all_changed || font_changed)
    {
        long long new_fn_id = install_font(state->getFont());

        if(!(new_fn_id == cur_fn_id))
        {
            close_line = true;
            cur_fn_id = new_fn_id;
        }

        if(!_equal(cur_font_size, state->getFontSize()))
        {
            cur_font_size = state->getFontSize();
            need_rescale_font = true;
        }
    }  

    // TODO
    // Rise, HorizScale etc
    if(all_changed || text_mat_changed || ctm_changed)
    {
        double new_ctm[6];
        double * m1 = state->getCTM();
        double * m2 = state->getTextMat();
        new_ctm[0] = m1[0] * m2[0] + m1[2] * m2[1];
        new_ctm[1] = m1[1] * m2[0] + m1[3] * m2[1];
        new_ctm[2] = m1[0] * m2[2] + m1[2] * m2[3];
        new_ctm[3] = m1[1] * m2[2] + m1[3] * m2[3];
        new_ctm[4] = new_ctm[5] = 0;

        if(!_tm_equal(new_ctm, cur_ctm))
        {
            need_rescale_font = true;
            memcpy(cur_ctm, new_ctm, sizeof(cur_ctm));
        }
    }

    if(need_rescale_font)
    {
        double new_draw_ctm[6];
        memcpy(new_draw_ctm, cur_ctm, sizeof(new_draw_ctm));

        draw_scale = std::sqrt(new_draw_ctm[2] * new_draw_ctm[2] + new_draw_ctm[3] * new_draw_ctm[3]);

        double new_draw_font_size = cur_font_size;
        if(_is_positive(draw_scale))
        {
            new_draw_font_size *= draw_scale;
            for(int i = 0; i < 4; ++i)
                new_draw_ctm[i] /= draw_scale;
        }
        else
        {
            draw_scale = 1.0;
        }

        if(!(_equal(new_draw_font_size, draw_font_size)))
        {
            draw_font_size = new_draw_font_size;
            cur_fs_id = install_font_size(draw_font_size);
            close_line = true;
        }
        if(!(_tm_equal(new_draw_ctm, draw_ctm)))
        {
            memcpy(draw_ctm, new_draw_ctm, sizeof(draw_ctm));
            cur_tm_id = install_transform_matrix(draw_ctm);
            close_line = true;
        }
    }

    // TODO: track these 
    /*
    if(!(_equal(s1->getCharSpace(), s2->getCharSpace()) && _equal(s1->getWordSpace(), s2->getWordSpace())
         && _equal(s1->getHorizScaling(), s2->getHorizScaling())))
            return false;
            */

    reset_state_track();

    if(close_line)
        close_cur_line();
}

void HTMLRenderer::reset_state_track()
{
    all_changed = false;
    text_pos_changed = false;
    ctm_changed = false;
    text_mat_changed = false;
    font_changed = false;
    color_changed = false;
}
