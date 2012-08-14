/*
 * general.cc
 *
 * Hanlding general stuffs
 *
 * TODO: better name for this file?
 *
 * by WangLu
 * 2012.08.14
 */

#include <iostream>

#include <boost/format.hpp>
#include <boost/filesystem/fstream.hpp>

#include <splash/SplashBitmap.h>

#include "HTMLRenderer.h"
#include "BackgroundRenderer.h"
#include "config.h"

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
{ }

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








