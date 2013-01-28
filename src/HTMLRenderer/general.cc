/*
 * general.cc
 *
 * Handling general stuffs
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 * 2012.08.14
 */

#include <cstdio>
#include <ostream>
#include <cmath>
#include <algorithm>
#include <vector>

#include <GlobalParams.h>

#include "HTMLRenderer.h"
#include "TextLineBuffer.h"
#include "pdf2htmlEX-config.h"
#include "BackgroundRenderer/BackgroundRenderer.h"
#include "util/namespace.h"
#include "util/ffw.h"
#include "util/base64stream.h"
#include "util/math.h"
#include "util/path.h"

namespace pdf2htmlEX {

using std::fixed;
using std::flush;
using std::ostream;
using std::max;
using std::min_element;
using std::vector;
using std::abs;
using std::cerr;
using std::endl;

HTMLRenderer::HTMLRenderer(const Param * param)
    :OutputDev()
    ,line_opened(false)
    ,text_line_buf(new TextLineBuffer(this))
    ,preprocessor(param)
	,tmp_files(*param)
    ,param(param)
{
    if(!(param->debug))
    {
        //disable error messages of poppler
        globalParams->setErrQuiet(gTrue);
    }

    ffw_init(param->debug);
    cur_mapping = new int32_t [0x10000];
    cur_mapping2 = new char* [0x100];
    width_list = new int [0x10000];
}

HTMLRenderer::~HTMLRenderer()
{ 
    delete text_line_buf;
    ffw_finalize();
    delete [] cur_mapping;
    delete [] cur_mapping2;
    delete [] width_list;
}

void HTMLRenderer::process(PDFDoc *doc)
{
    cur_doc = doc;
    cur_catalog = doc->getCatalog();
    xref = doc->getXRef();

    pre_process(doc);

    BackgroundRenderer * bg_renderer = nullptr;
    if(param->process_nontext)
    {
        bg_renderer = new BackgroundRenderer(this, param);
        bg_renderer->startDoc(doc);
    }

    int page_count = (param->last_page - param->first_page + 1);
    for(int i = param->first_page; i <= param->last_page ; ++i) 
    {
        cerr << "Working: " << (i-param->first_page) << "/" << page_count << '\r' << flush;

        if(param->split_pages)
        {
            auto page_fn = str_fmt("%s/%s%d.page", param->dest_dir.c_str(), param->output_filename.c_str(), i);
            f_pages.fs.open((char*)page_fn, ofstream::binary); 
            if(!f_pages.fs)
                throw string("Cannot open ") + (char*)page_fn + " for writing";
            set_stream_flags(f_pages.fs);
        }

        if(param->process_nontext)
        {
            auto fn = str_fmt("%s/p%x.png", (param->single_html ? param->tmp_dir : param->dest_dir).c_str(), i);
            if(param->single_html)
                tmp_files.add((char*)fn);

            bg_renderer->render_page(doc, i, (char*)fn);
        }

        doc->displayPage(this, i, 
                text_zoom_factor() * DEFAULT_DPI, text_zoom_factor() * DEFAULT_DPI,
                0, 
                (param->use_cropbox == 0), 
                false, false,
                nullptr, nullptr, nullptr, nullptr);

        if(param->split_pages)
        {
            f_pages.fs.close();
        }
    }
    if(page_count >= 0)
        cerr << "Working: " << page_count << "/" << page_count;
    cerr << endl;

    post_process();

    if(bg_renderer)
        delete bg_renderer;

    cerr << endl;
}

void HTMLRenderer::setDefaultCTM(double *ctm)
{
    memcpy(default_ctm, ctm, sizeof(default_ctm));
}

void HTMLRenderer::startPage(int pageNum, GfxState *state) 
{
    startPage(pageNum, state, nullptr);
}

void HTMLRenderer::startPage(int pageNum, GfxState *state, XRef * xref) 
{
    this->pageNum = pageNum;
    this->pageWidth = state->getPageWidth();
    this->pageHeight = state->getPageHeight();

    assert((!line_opened) && "Open line in startPage detected!");

    f_pages.fs 
        << "<div class=\"d\" style=\"width:" 
            << (pageWidth) << "px;height:" 
            << (pageHeight) << "px;\">"
        << "<div id=\"p" << pageNum << "\" data-page-no=\"" << pageNum << "\" class=\"p\">"
        << "<div class=\"b\" style=\"";

    if(param->process_nontext)
    {
        f_pages.fs << "background-image:url(";

        {
            if(param->single_html)
            {
                auto path = str_fmt("%s/p%x.png", param->tmp_dir.c_str(), pageNum);
                ifstream fin((char*)path, ifstream::binary);
                if(!fin)
                    throw string("Cannot read background image ") + (char*)path;
                f_pages.fs << "'data:image/png;base64," << base64stream(fin) << "'";
            }
            else
            {
                f_pages.fs << str_fmt("p%x.png", pageNum);
            }
        }

        f_pages.fs << ");background-position:0 0;background-size:" << pageWidth << "px " << pageHeight << "px;background-repeat:no-repeat;";
    }

    f_pages.fs << "\">";
    draw_text_scale = 1.0;

    cur_font_info = install_font(nullptr);
    cur_font_size = draw_font_size = 0;
    cur_fs_id = install_font_size(cur_font_size);
    
    memcpy(cur_text_tm, ID_MATRIX, sizeof(cur_text_tm));
    memcpy(draw_text_tm, ID_MATRIX, sizeof(draw_text_tm));
    cur_ttm_id = install_transform_matrix(draw_text_tm);

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
    close_text_line();

    // process links before the page is closed
    cur_doc->processLinks(this, pageNum);

    // close box
    f_pages.fs << "</div>";

    // dump info for js
    // TODO: create a function for this
    // BE CAREFUL WITH ESCAPES
    f_pages.fs << "<div class=\"j\" data-data='{";
    
    //default CTM
    f_pages.fs << "\"ctm\":[";
    for(int i = 0; i < 6; ++i)
    {
        if(i > 0) f_pages.fs << ",";
        f_pages.fs << round(default_ctm[i]);
    }
    f_pages.fs << "]";

    f_pages.fs << "}'></div>";
    
    // close page
    f_pages.fs << "</div></div>" << endl;
}

void HTMLRenderer::pre_process(PDFDoc * doc)
{
    preprocessor.process(doc);

    /*
     * determine scale factors
     */
    {
        double zoom = 1.0;

        vector<double> zoom_factors;
        
        if(is_positive(param->zoom))
        {
            zoom_factors.push_back(param->zoom);
        }

        if(is_positive(param->fit_width))
        {
            zoom_factors.push_back((param->fit_width) / preprocessor.get_max_width());
        }

        if(is_positive(param->fit_height))
        {
            zoom_factors.push_back((param->fit_height) / preprocessor.get_max_height());
        }

        if(zoom_factors.empty())
        {
            zoom = 1.0;
        }
        else
        {
            zoom = *min_element(zoom_factors.begin(), zoom_factors.end());
        }
        
        text_scale_factor1 = max<double>(zoom, param->font_size_multiplier);  
        text_scale_factor2 = zoom / text_scale_factor1;
    }

    // we may output utf8 characters, so always use binary
    {
        /*
         * If single-html && !split-pages
         * we have to keep the generated css file into a temporary place
         * and embed it into the main html later
         *
         *
         * If single-html && split-page
         * as there's no place to embed the css file, just leave it alone (into param->dest_dir)
         *
         * If !single-html
         * leave it in param->dest_dir
         */

        auto fn = (param->single_html && (!param->split_pages))
            ? str_fmt("%s/__css", param->tmp_dir.c_str())
            : str_fmt("%s/%s", param->dest_dir.c_str(), param->css_filename.c_str());

        if(param->single_html && (!param->split_pages))
            tmp_files.add((char*)fn);

        f_css.path = (char*)fn,
        f_css.fs.open(f_css.path, ofstream::binary);
        if(!f_css.fs)
            throw string("Cannot open ") + (char*)fn + " for writing";
        set_stream_flags(f_css.fs);
    }

    // if split-pages is specified, open & close the file in the process loop
    // if not, open the file here:
    if(!param->split_pages)
    {
        /*
         * If single-html
         * we have to keep the html file (for page) into a temporary place
         * because we'll have to embed css before it
         *
         * Otherwise just generate it 
         */
        auto fn = str_fmt("%s/__pages", param->tmp_dir.c_str());
        tmp_files.add((char*)fn);

        f_pages.path = (char*)fn;
        f_pages.fs.open(f_pages.path, ofstream::binary); 
        if(!f_pages.fs)
            throw string("Cannot open ") + (char*)fn + " for writing";
        set_stream_flags(f_pages.fs);
    }
}

void HTMLRenderer::post_process()
{
    // close files
    f_pages.fs.close(); 
    f_css.fs.close();

    //only when split-page == 0, do we have some work left to do
    if(param->split_pages)
        return;

    ofstream output;
    {
        auto fn = str_fmt("%s/%s", param->dest_dir.c_str(), param->output_filename.c_str());
        output.open((char*)fn, ofstream::binary);
        if(!output)
            throw string("Cannot open ") + (char*)fn + " for writing";
        set_stream_flags(output);
    }

    // apply manifest
    ifstream manifest_fin((char*)str_fmt("%s/%s", param->data_dir.c_str(), MANIFEST_FILENAME.c_str()), ifstream::binary);
    if(!manifest_fin)
        throw "Cannot open the manifest file";

    bool embed_string = false;
    string line;
    while(getline(manifest_fin, line))
    {
        if(line == "\"\"\"")
        {
            embed_string = !embed_string;
            continue;
        }

        if(embed_string)
        {
            output << line << endl;
            continue;
        }

        if(line.empty() || line[0] == '#')
            continue;


        if(line[0] == '@')
        {
            embed_file(output, param->data_dir + "/" + line.substr(1), "", true);
            continue;
        }

        if(line[0] == '$')
        {
            if(line == "$css")
            {
                embed_file(output, f_css.path, ".css", false);
            }
            else if (line == "$pages")
            {
                ifstream fin(f_pages.path, ifstream::binary);
                if(!fin)
                    throw "Cannot open read the pages";
                output << fin.rdbuf();
            }
            else
            {
                cerr << "Warning: unknown line in manifest: " << line << endl;
            }
            continue;
        }

        cerr << "Warning: unknown line in manifest: " << line << endl;
    }
}

void HTMLRenderer::set_stream_flags(std::ostream & out)
{
    // we output all ID's in hex
    // browsers are not happy with scientific notations
    out << hex << fixed;
}

void HTMLRenderer::embed_file(ostream & out, const string & path, const string & type, bool copy)
{
    string fn = get_filename(path);
    string suffix = (type == "") ? get_suffix(fn) : type; 
    
    auto iter = EMBED_STRING_MAP.find(make_pair(suffix, (bool)param->single_html));
    if(iter == EMBED_STRING_MAP.end())
    {
        cerr << "Warning: unknown suffix: " << suffix << endl;
        return;
    }
    
    if(param->single_html)
    {
        ifstream fin(path, ifstream::binary);
        if(!fin)
            throw string("Cannot open file ") + path + " for embedding";
        out << iter->second.first << endl
            << fin.rdbuf()
            << iter->second.second << endl;
    }
    else
    {
        out << iter->second.first
            << fn
            << iter->second.second << endl;

        if(copy)
        {
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw string("Cannot copy file: ") + path;
            auto out_path = param->dest_dir + "/" + fn;
            ofstream out(out_path, ofstream::binary);
            if(!out)
                throw string("Cannot open file ") + path + " for embedding";
            out << fin.rdbuf();
        }
    }
}

const std::string HTMLRenderer::MANIFEST_FILENAME = "manifest";

}// namespace pdf2htmlEX
