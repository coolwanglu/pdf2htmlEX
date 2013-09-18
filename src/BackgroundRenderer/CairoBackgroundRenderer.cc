/*
 * CairoBackgroundRenderer.cc
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */

#include <string>
#include <fstream>


#include "pdf2htmlEX-config.h"

#include "Base64Stream.h"

#if ENABLE_SVG

#include "CairoBackgroundRenderer.h"

namespace pdf2htmlEX {

using std::string;
using std::ifstream;

void CairoBackgroundRenderer::drawChar(GfxState *state, double x, double y,
        double dx, double dy,
        double originX, double originY,
        CharCode code, int nBytes, Unicode *u, int uLen)
{
    //    CairoOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code, nBytes, u, uLen);
}

static GBool annot_cb(Annot *, void *) {
    return false;
};

void CairoBackgroundRenderer::render_page(PDFDoc * doc, int pageno)
{
    auto fn = html_renderer->str_fmt("%s/bg%x.svg", (param.embed_image ? param.tmp_dir : param.dest_dir).c_str(), pageno);
    if(param.embed_image)
        html_renderer->tmp_files.add((char*)fn);

    double page_width;
    double page_height;
    if(param.use_cropbox)
    {
        page_width = doc->getPageCropWidth(pageno);
        page_height = doc->getPageCropHeight(pageno);
    }
    else
    {
        page_width = doc->getPageMediaWidth(pageno);
        page_height = doc->getPageMediaHeight(pageno);
    }

    surface = cairo_svg_surface_create((char*)fn, page_width, page_height);
    cairo_svg_surface_restrict_to_version(surface, CAIRO_SVG_VERSION_1_2);
    cairo_surface_set_fallback_resolution(surface, param.h_dpi, param.v_dpi);

    cairo_t * cr = cairo_create(surface);
    setCairo(cr);
    setPrinting(false); // TODO, check the parameter
    cairo_save(cr);

    // TODO apply crop box
   
    doc->displayPage(this, pageno, DEFAULT_DPI, DEFAULT_DPI,
            0, 
            (!(param.use_cropbox)),
            false, 
            false,
            nullptr, nullptr, &annot_cb, nullptr);

    cairo_restore(cr);
    setCairo(nullptr);
    
    {
        auto status = cairo_status(cr);
        cairo_destroy(cr);
        if(status)
            throw string("Cairo error: ") + cairo_status_to_string(status);
    }

//    cairo_surface_show_page(surface);
    cairo_surface_finish(surface);
    {
        auto status = cairo_surface_status(surface);
        cairo_surface_destroy(surface);
        surface = nullptr;
        if(status)
            throw string("Error in cairo: ") + cairo_status_to_string(status);
    }
}

void CairoBackgroundRenderer::embed_image(int pageno)
{
    {
        auto fn = html_renderer->str_fmt("%s/bg%x.png", (param.embed_image ? param.tmp_dir : param.dest_dir).c_str(), pageno);
        if(param.embed_image)
            html_renderer->tmp_files.add((char*)fn);
    }

    auto & f_page = *(html_renderer->f_curpage);
    
    f_page << "<img class=\"" << CSS::FULL_BACKGROUND_IMAGE_CN 
        << "\" alt=\"\" src=\"";

    if(param.embed_image)
    {
        auto path = html_renderer->str_fmt("%s/bg%x.svg", param.tmp_dir.c_str(), pageno);
        ifstream fin((char*)path, ifstream::binary);
        if(!fin)
            throw string("Cannot read background image ") + (char*)path;
        f_page << "data:image/svg+xml;base64," << Base64Stream(fin);
    }
    else
    {
        f_page << (char*)html_renderer->str_fmt("bg%x.svg", pageno);
    }
    f_page << "\"/>";
}

} // namespace pdf2htmlEX

#endif // ENABLE_SVG

