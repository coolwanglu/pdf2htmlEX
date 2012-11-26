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

#include "HTMLRenderer.h"
#include "TextState.h"
#include "BackgroundRenderer.h"
#include "namespace.h"
#include "ffw.h"
#include "pdf2htmlEX-config.h"

namespace pdf2htmlEX {

using std::fixed;
using std::flush;
using std::ostream;
using std::max;
using std::min_element;
using std::vector;
using std::abs;

static void dummy(void *, enum ErrorCategory, int pos, char *)
{
}

HTMLRenderer::HTMLRenderer(const Param * param)
    :OutputDev()
    ,line_opened(false)
    ,line_buf(this)
    ,preprocessor(param)
	,tmp_files(*param)
	,device(*param, tmp_files)
    ,image_count(0)
    ,param(param)
{
    if(!(param->debug))
    {
        //disable error function of poppler
        setErrorCallback(&dummy, nullptr);
    }

    ffw_init(param->debug);
    cur_mapping = new int32_t [0x10000];
    cur_mapping2 = new char* [0x100];
    width_list = new int [0x10000];
}

HTMLRenderer::~HTMLRenderer()
{ 
    ffw_finalize();
    delete [] cur_mapping;
    delete [] cur_mapping2;
    delete [] width_list;
}

void HTMLRenderer::process(PDFDoc *doc)
{
    cur_doc = doc;
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

		auto fn = device.page_start( i );
		tmp_files.add(fn.c_str());

        if( !fn.empty() )
            bg_renderer->render_page(doc, i, fn.c_str() );

        doc->displayPage(this, i, 
                text_zoom_factor() * DEFAULT_DPI, text_zoom_factor() * DEFAULT_DPI,
                0, true, false, false,
                nullptr, nullptr, nullptr, nullptr);

		device.page_end();
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
    this->pageNum = pageNum;
    this->pageWidth = state->getPageWidth();
    this->pageHeight = state->getPageHeight();

    assert((!line_opened) && "Open line in startPage detected!");

	device.page_header( pageWidth, pageHeight, pageNum );

    draw_text_scale = 1.0;

    cur_font_info = install_font(nullptr);
    cur_font_size = draw_font_size = 0;
    cur_fs_id = install_font_size(cur_font_size);
    
    memcpy(cur_text_tm, id_matrix, sizeof(cur_text_tm));
    memcpy(draw_text_tm, id_matrix, sizeof(draw_text_tm));
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

	device.page_footer( default_ctm );
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
        
        if(_is_positive(param->zoom))
        {
            zoom_factors.push_back(param->zoom);
        }

        if(_is_positive(param->fit_width))
        {
            zoom_factors.push_back((param->fit_width) / preprocessor.get_max_width());
        }

        if(_is_positive(param->fit_height))
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

	device.document_start();
}

void HTMLRenderer::post_process()
{
	device.document_end();
}


}// namespace pdf2htmlEX
