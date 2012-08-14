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
#include <boost/filesystem/fstream.hpp>
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
#include "config.h"

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
}

HTMLRenderer::~HTMLRenderer()
{
}

void HTMLRenderer::process(PDFDoc *doc)
{
    std::cerr << "Processing Text: ";
    write_html_head();
    xref = doc->getXRef();
    for(int i = param->first_page; i <= param->last_page ; ++i) 
    {
        doc->displayPage(this, i, param->h_dpi, param->v_dpi,
                0, true, false, false,
                nullptr, nullptr, nullptr, nullptr);

        std::cerr << ".";
        std::cerr.flush();
    }
    write_html_tail();
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

void HTMLRenderer::write_html_head()
{
    html_fout << boost::filesystem::ifstream(PDF2HTMLEX_LIB_PATH / "head.html", ifstream::binary).rdbuf();
}

void HTMLRenderer::write_html_tail()
{
    html_fout << boost::filesystem::ifstream(PDF2HTMLEX_LIB_PATH / "tail.html", ifstream::binary).rdbuf();
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





