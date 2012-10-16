/*
 * CairoBackgroundRenderer.cc
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */

#include "pdf2htmlEX-config.h"

#if HAVE_CAIRO

#include <fstream>

#include "CairoBackgroundRenderer.h"
#include <cairo-svg.h>

namespace pdf2htmlEX {

using std::ofstream;

static cairo_suface_t write_to_file (void * out, const unsigned char * data, unsigned int length)
{
    ofstream * fout = static_cast<ofstream*>(out);
    fout->write(data, length);
    return (fout) ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

void CairoBackgroundRenderer::pre_process(PDFDoc * doc)
{
    startDoc(doc);
    out = new ofstream(param->tmp_dir + "/__bg_tmp", ofstream::binary);
    surface = cairo_svg_surface_create_for_stream(write_to_file, out,
}

void CairoBackgroundRenderer::drawChar(GfxState *state, double x, double y,
        double dx, double dy,
        double originX, double originY,
        CharCode code, int nBytes, Unicode *u, int uLen)
{
    //    CairoOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code, nBytes, u, uLen);
}

void CairoBackgroundRenderer::render_page(PDFDoc * doc, int pageno, const std::string & filename)
{
}

} // namespace pdf2htmlEX

#endif // HAVE_CAIRO

