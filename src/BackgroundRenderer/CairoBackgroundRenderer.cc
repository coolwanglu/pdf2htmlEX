/*
 * CairoBackgroundRenderer.cc
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <string>
#include <fstream>


#include "pdf2htmlEX-config.h"

#include "Base64Stream.h"

#if ENABLE_SVG

#include "CairoBackgroundRenderer.h"
#include "SplashBackgroundRenderer.h"

namespace pdf2htmlEX {

using std::string;
using std::ifstream;

CairoBackgroundRenderer::CairoBackgroundRenderer(HTMLRenderer * html_renderer, const Param & param)
      : CairoOutputDev()
      , html_renderer(html_renderer)
      , param(param)
      , surface(nullptr)
      , use_bitmap(false)
{
    if (param.svg_nodes_limit > 0)
    {
        this->bitmap_renderer = new SplashBackgroundRenderer(html_renderer, param);
    }
    else
        this->bitmap_renderer = nullptr;
}

void CairoBackgroundRenderer::drawChar(GfxState *state, double x, double y,
        double dx, double dy,
        double originX, double originY,
        CharCode code, int nBytes, Unicode *u, int uLen)
{
    // draw characters as image when
    // - in fallback mode
    // - OR there is special filling method
    // - OR using a writing mode font
    // - OR using a Type 3 font while param.process_type3 is not enabled
    if((param.fallback)
       || ( (state->getFont()) 
            && ( (state->getFont()->getWMode())
                 || ((state->getFont()->getType() == fontType3) && (!param.process_type3))
               )
          )
      )
    {
        CairoOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code,nBytes,u,uLen);
    }
}

void CairoBackgroundRenderer::init(PDFDoc * doc)
{
    startDoc(doc);
    if (this->bitmap_renderer != nullptr)
          this->bitmap_renderer->init(doc);
}

static GBool annot_cb(Annot *, void * pflag) {
    return (*((bool*)pflag)) ? gTrue : gFalse;
};

void CairoBackgroundRenderer::render_page(PDFDoc * doc, int pageno)
{
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

    auto fn = html_renderer->str_fmt("%s/bg%x.svg", (param.embed_image ? param.tmp_dir : param.dest_dir).c_str(), pageno);
    if(param.embed_image)
        html_renderer->tmp_files.add((char*)fn);

    surface = cairo_svg_surface_create((char*)fn, page_width * param.h_dpi / DEFAULT_DPI, page_height * param.v_dpi / DEFAULT_DPI);
    cairo_svg_surface_restrict_to_version(surface, CAIRO_SVG_VERSION_1_2);
    cairo_surface_set_fallback_resolution(surface, param.h_dpi, param.v_dpi);

    cairo_t * cr = cairo_create(surface);
    setCairo(cr);

    bool process_annotation = param.process_annotation;
    doc->displayPage(this, pageno, param.h_dpi, param.v_dpi,
            0, 
            (!(param.use_cropbox)),
            false, 
            false,
            nullptr, nullptr, &annot_cb, &process_annotation);

    setCairo(nullptr);
    
    {
        auto status = cairo_status(cr);
        cairo_destroy(cr);
        if(status)
            throw string("Cairo error: ") + cairo_status_to_string(status);
    }

    cairo_surface_finish(surface);
    {
        auto status = cairo_surface_status(surface);
        cairo_surface_destroy(surface);
        surface = nullptr;
        if(status)
            throw string("Error in cairo: ") + cairo_status_to_string(status);
    }

    //check node count in the svg file, fall back to bitmap_renderer if necessary.
    if (param.svg_nodes_limit > 0)
    {
        int n = 0;
        char c;
        ifstream svgfile((char*)fn);
        //count of '<' in the file should be an approximation of node count.
        while(svgfile >> c)
        {
            if (c == '<')
                ++n;
        }
        svgfile.close();
        if (n > param.svg_nodes_limit)
        {
            html_renderer->tmp_files.add((char*)fn);
            use_bitmap = true;
            bitmap_renderer->render_page(doc, pageno);
        }
        else
            use_bitmap = false;
    }
}

void CairoBackgroundRenderer::embed_image(int pageno)
{
    if (use_bitmap)
    {
        bitmap_renderer->embed_image(pageno);
        return;
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

