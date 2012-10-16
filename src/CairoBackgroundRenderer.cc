/*
 * CairoBackgroundRenderer.cc
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 *
 * Many lines of code are from pdftocairo in poppler
 */

#include "pdf2htmlEX-config.h"

#if HAVE_CAIRO

#include <fstream>
#include <vector>

#include <cairo.h>
#include <cairo-svg.h>

#include <goo/PNGWriter.h>

#include "CairoBackgroundRenderer.h"

namespace pdf2htmlEX {

using std::ofstream;
using std::string;
using std::vector;

void close_surface(cairo_surface_t *& surface)
{
    if(surface)
    {
        cairo_surface_finish(surface);
        auto status = cairo_surface_status(surface);
        if(status)
            throw string("cairo error: ") + cairo_status_to_string(status);
        cairo_surface_destroy(surface);
        surface = nullptr;
    }
}

CairoBackgroundRenderer::~CairoBackgroundRenderer (void)
{
    close_surface(surface);
    if(out)
    {
        delete out;
        out = nullptr;
    }
}

static cairo_status_t write_to_file (void * out, const unsigned char * data, unsigned int length)
{
    ofstream * fout = static_cast<ofstream*>(out);
    fout->write((const char*)data, length);
    return (fout) ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

void CairoBackgroundRenderer::pre_process(PDFDoc * doc)
{
    CairoOutputDev::startDoc(doc);
}

void CairoBackgroundRenderer::startPage(int pageNum, GfxState * state)
{
    close_surface(surface);
    if(param->svg_draw)
    {
        if(out)
        {
            delete out;
            out = nullptr;
        }
        out = new ofstream(cur_page_filename, ofstream::binary);
        surface = cairo_svg_surface_create_for_stream(write_to_file, out,
                state->getPageWidth(), state->getPageHeight());
        cairo_svg_surface_restrict_to_version (surface, CAIRO_SVG_VERSION_1_2); 
        cairo_surface_set_fallback_resolution (surface, param->h_dpi, param->v_dpi);
    }
    else
    {
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, state->getPageWidth(), state->getPageHeight());
    }

    context = cairo_create(surface);
    CairoOutputDev::setCairo(context);
    CairoOutputDev::setPrinting(param->svg_draw);

    CairoOutputDev::startPage(pageNum, state);
}

void CairoBackgroundRenderer::endPage(void)
{
    CairoOutputDev::endPage();

    CairoOutputDev::setCairo(nullptr);

    auto status = cairo_status(context);
    if(status)
        throw string("cairo error: ") + cairo_status_to_string(status);
    cairo_destroy(context);
    context = nullptr;
}

void CairoBackgroundRenderer::drawChar(GfxState *state, double x, double y,
        double dx, double dy,
        double originX, double originY,
        CharCode code, int nBytes, Unicode *u, int uLen)
{
    //    CairoOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code, nBytes, u, uLen);
}

void CairoBackgroundRenderer::dump(void)
{
    if(param->svg_draw)
    {
        cairo_surface_show_page(surface);
    }
    else
    {
#ifndef ENABLE_LIBPNG
        throw string("poppler was not built with libpng!");
#endif
        auto writer = new PNGWriter(PNGWriter::RGBA);
        FILE * f = fopen(cur_page_filename.c_str(), "wb");
        if(!f)
            throw string("Cannot open file for writing: ") + cur_page_filename;

        auto height = cairo_image_surface_get_height(surface);
        auto width = cairo_image_surface_get_width(surface);
        auto stride = cairo_image_surface_get_stride(surface);
        auto data = cairo_image_surface_get_data(surface);

        if(!writer->init(f, width, height, param->h_dpi, param->v_dpi))
            throw string("Cannot initialize PNGWriter");

        vector<unsigned char> row(width * 4);
        for (int y = 0; y < height; y++ ) {
            uint32_t *pixel = (uint32_t *) (data + y*stride);
            unsigned char *rowp = &row.front();
            for (int x = 0; x < width; x++, pixel++) {
                // unpremultiply into RGBA format
                uint8_t a;
                a = (*pixel & 0xff000000) >> 24;
                if (a == 0) {
                    *rowp++ = 0;
                    *rowp++ = 0;
                    *rowp++ = 0;
                } else {
                    *rowp++ = (((*pixel & 0xff0000) >> 16) * 255 + a / 2) / a;
                    *rowp++ = (((*pixel & 0x00ff00) >>  8) * 255 + a / 2) / a;
                    *rowp++ = (((*pixel & 0x0000ff) >>  0) * 255 + a / 2) / a;
                }
                *rowp++ = a;
            }
        }
        {
            auto p = &row.front();
            writer->writeRow(&p);
        }
        writer->close();
        delete writer;
        fclose(f);
    }
}

} // namespace pdf2htmlEX

#endif // HAVE_CAIRO

