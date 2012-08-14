/*
 * general.cc
 *
 * Hanlding general stuffs
 *
 * by WangLu
 * 2012.08.14
 */

#include <iomanip>

#include <splash/SplashBitmap.h>

#include "HTMLRenderer.h"
#include "BackgroundRenderer.h"
#include "config.h"
#include "namespace.h"

using std::flush;

HTMLRenderer::HTMLRenderer(const Param * param)
    :line_opened(false)
    ,image_count(0)
    ,param(param)
    ,dest_dir(param->dest_dir)
    ,tmp_dir(TMP_DIR)
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
    cerr << "Working: ";

    xref = doc->getXRef();

    BackgroundRenderer * bg_renderer = nullptr;

    if(param->process_nontext)
    {
        // Render non-text objects as image
        // copied from poppler
        SplashColor color;
        color[0] = color[1] = color[2] = 255;

        bg_renderer = new BackgroundRenderer(splashModeRGB8, 4, gFalse, color);
        bg_renderer->startDoc(doc);
    }

    pre_process();
    for(int i = param->first_page; i <= param->last_page ; ++i) 
    {
        if(param->process_nontext)
        {
            doc->displayPage(bg_renderer, i, param->h_dpi2, param->v_dpi2,
                    0, true, false, false,
                    nullptr, nullptr, nullptr, nullptr);
            bg_renderer->getBitmap()->writeImgFile(splashFormatPng, (char*)(working_dir() / (format("p%|1$x|.png")%i).str()).c_str(), param->h_dpi2, param->v_dpi2);
        }

        doc->displayPage(this, i, param->h_dpi, param->v_dpi,
                0, true, false, false,
                nullptr, nullptr, nullptr, nullptr);

        cerr << "." << flush;
    }
    post_process();

    if(bg_renderer)
        delete bg_renderer;

    cerr << endl;
}

void HTMLRenderer::pre_process()
{
    // we may output utf8 characters, so use binary
    html_fout.open(working_dir() / param->output_filename, ofstream::binary); 
    allcss_fout.open(working_dir() / "all.css", ofstream::binary);

    if(!param->single_html)
    {
        html_fout << ifstream(PDF2HTMLEX_LIB_PATH / "head.html", ifstream::binary).rdbuf();
        html_fout << "<link rel=\"stylesheet\" type=\"text/css\" href=\"all.css\"/>" << endl;
        html_fout << ifstream(PDF2HTMLEX_LIB_PATH / "neck.html", ifstream::binary).rdbuf();
    }

    allcss_fout << ifstream(PDF2HTMLEX_LIB_PATH / "base.css", ifstream::binary).rdbuf();
}

void HTMLRenderer::post_process()
{
    if(!param->single_html)
    {
        html_fout << ifstream(PDF2HTMLEX_LIB_PATH / "tail.html", ifstream::binary).rdbuf();
    }

    html_fout.close();
    allcss_fout.close();

    if(param->single_html)
    {
        process_single_html();
    }
}

void HTMLRenderer::startPage(int pageNum, GfxState *state) 
{
    this->pageNum = pageNum;
    this->pageWidth = state->getPageWidth();
    this->pageHeight = state->getPageHeight();

    assert(!line_opened);

    html_fout << format("<div id=\"p%|1$x|\" class=\"p\">") % pageNum << endl;

    allcss_fout << format("#p%|1$x|{width:%2%px;height:%3%px;") % pageNum % pageWidth % pageHeight;

    allcss_fout << "background-image:url(";

    const std::string fn = (format("p%|1$x|.png") % pageNum).str();
    if(param->single_html)
    {
        auto path = tmp_dir / fn;
        allcss_fout << "'data:image/png;base64," << base64_filter(ifstream(path, ifstream::binary)) << "'";
    }
    else
    {
        allcss_fout << fn;
    }
    
    allcss_fout << format(");background-position:0 0;background-size:%1%px %2%px;background-repeat:no-repeat;}") % pageWidth % pageHeight;
            
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

void HTMLRenderer::process_single_html()
{
    ofstream out (dest_dir / param->output_filename, ofstream::binary);

    out << ifstream(PDF2HTMLEX_LIB_PATH / "head.html", ifstream::binary).rdbuf();

    out << "<style type=\"text/css\">" << endl;
    out << ifstream(tmp_dir / "all.css", ifstream::binary).rdbuf();
    out << "</style>" << endl;

    out << ifstream(PDF2HTMLEX_LIB_PATH / "neck.html", ifstream::binary).rdbuf();

    out << ifstream(tmp_dir / param->output_filename, ifstream::binary).rdbuf();

    out << ifstream(PDF2HTMLEX_LIB_PATH / "tail.html", ifstream::binary).rdbuf();
}






