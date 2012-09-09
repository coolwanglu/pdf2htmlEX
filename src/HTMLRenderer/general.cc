/*
 * general.cc
 *
 * Hanlding general stuffs
 *
 * by WangLu
 * 2012.08.14
 */

#include <boost/filesystem.hpp>

#include <splash/SplashBitmap.h>

#include "HTMLRenderer.h"
#include "BackgroundRenderer.h"
#include "config.h"
#include "namespace.h"
#include "ff/ff.h"

using std::fixed;
using std::flush;
using std::function;
using boost::filesystem::remove;
using boost::filesystem::filesystem_error;

static void dummy(void *, ErrorCategory, int pos, char *)
{
}

HTMLRenderer::HTMLRenderer(const Param * param)
    :line_opened(false)
    ,line_buf(this)
    ,image_count(0)
    ,param(param)
    ,dest_dir(param->dest_dir)
    ,tmp_dir(param->tmp_dir)
{
    //disable error function of poppler
    setErrorCallback(&dummy, nullptr);

    ff_init();
    cur_mapping = new int32_t [0x10000];
    cur_mapping2 = new char* [0x100];
}

HTMLRenderer::~HTMLRenderer()
{ 
    ff_fin();
    clean_tmp_files();
    delete [] cur_mapping;
    delete [] cur_mapping2;
}

static GBool annot_cb(Annot *, void *) {
    return false;
};

void HTMLRenderer::process(PDFDoc *doc)
{
    xref = doc->getXRef();

    cerr << "Preprocessing: ";
    for(int i = param->first_page; i <= param->last_page ; ++i) 
    {
        doc->displayPage(&font_preprocessor, i, param->h_dpi, param->v_dpi,
                0, true, false, false,
                nullptr, nullptr, nullptr, nullptr);
        cerr << "." << flush;
    }
    cerr << endl;

    cerr << "Working: ";
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
            doc->displayPage(bg_renderer, i, param->h_dpi, param->v_dpi,
                    0, true, false, false,
                    nullptr, nullptr, &annot_cb, nullptr);

            {
                const char * fn = str_fmt("p%llx.png", i);
                bg_renderer->getBitmap()->writeImgFile(splashFormatPng, (char*)((param->single_html ? tmp_dir : dest_dir) / fn) .c_str(), param->h_dpi, param->v_dpi);
                if(param->single_html)
                    add_tmp_file(fn);
            }
        }

        doc->displayPage(this, i, param->zoom * DEFAULT_DPI, param->zoom * DEFAULT_DPI,
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
    if(param->single_html)
    {
        // don't use output_file directly
        // otherwise it'll be a disaster when tmp_dir == dest_dir
        const string tmp_output_fn = param->output_filename + ".part";

        html_fout.open(tmp_dir / tmp_output_fn, ofstream::binary); 
        allcss_fout.open(tmp_dir / CSS_FILENAME, ofstream::binary);
        
        add_tmp_file(tmp_output_fn);
        add_tmp_file(CSS_FILENAME);
    }
    else
    {
        html_fout.open(dest_dir / param->output_filename, ofstream::binary); 
        allcss_fout.open(dest_dir / CSS_FILENAME, ofstream::binary);

        html_fout << ifstream(PDF2HTMLEX_DATA_PATH / HEAD_HTML_FILENAME, ifstream::binary).rdbuf();
        html_fout << "<link rel=\"stylesheet\" type=\"text/css\" href=\"" << CSS_FILENAME << "\"/>" << endl;
        html_fout << ifstream(PDF2HTMLEX_DATA_PATH / NECK_HTML_FILENAME, ifstream::binary).rdbuf();
    }

    html_fout << fixed << hex;
    allcss_fout << fixed << hex;

    allcss_fout << ifstream(PDF2HTMLEX_DATA_PATH / CSS_FILENAME, ifstream::binary).rdbuf(); 
}

void HTMLRenderer::post_process()
{
    if(!param->single_html)
    {
        html_fout << ifstream(PDF2HTMLEX_DATA_PATH / TAIL_HTML_FILENAME, ifstream::binary).rdbuf();
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

    assert((!line_opened) && "Open line in startPage detected!");

    html_fout << "<div id=\"p" << pageNum << "\" class=\"p\" style=\"width:" << pageWidth << "px;height:" << pageHeight << "px;";

    html_fout << "background-image:url(";

    {
        const char * fn = str_fmt("p%llx.png", pageNum);
        if(param->single_html)
        {
            auto path = tmp_dir / fn;
            html_fout << "'data:image/png;base64," << base64stream(ifstream(path, ifstream::binary)) << "'";
        }
        else
        {
            html_fout << fn;
        }
    }
    
    html_fout << ");background-position:0 0;background-size:" << pageWidth << "px " << pageHeight << "px;background-repeat:no-repeat;\">";
            
    draw_scale = 1.0;

    cur_font_info = install_font(nullptr);
    cur_font_size = draw_font_size = 0;
    cur_fs_id = install_font_size(cur_font_size);
    
    memcpy(cur_ctm, id_matrix, sizeof(cur_ctm));
    memcpy(draw_ctm, id_matrix, sizeof(draw_ctm));
    cur_tm_id = install_transform_matrix(draw_ctm);

    cur_letter_space = cur_word_space = 0;
    cur_ls_id = install_letter_space(cur_letter_space);
    cur_ws_id = install_word_space(cur_word_space);

    cur_color.r = cur_color.g = cur_color.b = 0;
    cur_color_id = install_color(&cur_color);

    cur_rise = 0;
    cur_rise_id = install_rise(cur_rise);

    cur_tx = cur_ty = 0;
    draw_tx = draw_ty = 0;

    reset_state_change();
    all_changed = true;
}

void HTMLRenderer::endPage() {
    close_line();
    // close page
    html_fout << "</div>" << endl;
}

void HTMLRenderer::process_single_html()
{
    ofstream out (dest_dir / param->output_filename, ofstream::binary);

    out << ifstream(PDF2HTMLEX_DATA_PATH / HEAD_HTML_FILENAME , ifstream::binary).rdbuf();

    out << "<style type=\"text/css\">" << endl;
    out << ifstream(tmp_dir / CSS_FILENAME, ifstream::binary).rdbuf();
    out << "</style>" << endl;

    out << ifstream(PDF2HTMLEX_DATA_PATH / NECK_HTML_FILENAME, ifstream::binary).rdbuf();

    out << ifstream(tmp_dir / (param->output_filename + ".part"), ifstream::binary).rdbuf();

    out << ifstream(PDF2HTMLEX_DATA_PATH / TAIL_HTML_FILENAME, ifstream::binary).rdbuf();
}

void HTMLRenderer::add_tmp_file(const string & fn)
{
    if(!param->clean_tmp)
        return;

    if(tmp_files.insert(fn).second && param->debug)
        cerr << "Add new temporary file: " << fn << endl;
}

void HTMLRenderer::clean_tmp_files()
{
    if(!param->clean_tmp)
        return;

    for(auto iter = tmp_files.begin(); iter != tmp_files.end(); ++iter)
    {
        try
        {
            auto fn = *iter;
            remove(tmp_dir / fn);
            if(param->debug)
                cerr << "Remove temporary file: " << fn << endl;
        }
        catch(const filesystem_error &)
        { }
    }
    try
    {
        remove(tmp_dir);
        if(param->debug)
            cerr << "Remove temporary directory: " << tmp_dir << endl;
    }
    catch(const filesystem_error &)
    { }
}

const std::string HTMLRenderer::HEAD_HTML_FILENAME = "head.html";
const std::string HTMLRenderer::NECK_HTML_FILENAME = "neck.html";
const std::string HTMLRenderer::TAIL_HTML_FILENAME = "tail.html";
const std::string HTMLRenderer::CSS_FILENAME = "all.css";
