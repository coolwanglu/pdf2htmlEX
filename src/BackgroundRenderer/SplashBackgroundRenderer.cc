/*
 * SplashBackgroundRenderer.cc
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <fstream>
#include <vector>
#include <memory>

#include <poppler-config.h>
#include <PDFDoc.h>
#include <goo/PNGWriter.h>
#include <goo/JpegWriter.h>

#include "Base64Stream.h"
#include "util/const.h"

#include "SplashBackgroundRenderer.h"

namespace pdf2htmlEX {

using std::string;
using std::ifstream;
using std::vector;
using std::unique_ptr;

const SplashColor SplashBackgroundRenderer::white = {255,255,255};

/*
 * SplashOutputDev::startPage would paint the whole page with the background color
 * And thus have modified region set to the whole page area
 * We do not want that.
 */
#if POPPLER_OLDER_THAN_0_23_0
void SplashBackgroundRenderer::startPage(int pageNum, GfxState *state)
{
    SplashOutputDev::startPage(pageNum, state);
#else
void SplashBackgroundRenderer::startPage(int pageNum, GfxState *state, XRef *xrefA)
{
    SplashOutputDev::startPage(pageNum, state, xrefA);
#endif
    clearModRegion();
}

void SplashBackgroundRenderer::drawChar(GfxState *state, double x, double y,
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
        SplashOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code,nBytes,u,uLen);
    }
}

void SplashBackgroundRenderer::init(PDFDoc * doc)
{
    startDoc(doc);
}

static GBool annot_cb(Annot *, void *) {
    return false;
};

void SplashBackgroundRenderer::render_page(PDFDoc * doc, int pageno)
{
    doc->displayPage(this, pageno, param.h_dpi, param.v_dpi,
            0, 
            (!(param.use_cropbox)),
            false, false,
            nullptr, nullptr, &annot_cb, nullptr);
}

void SplashBackgroundRenderer::embed_image(int pageno)
{
    // xmin->xmax is top->bottom
    int xmin, xmax, ymin, ymax;
    getModRegion(&xmin, &ymin, &xmax, &ymax);

    // dump the background image only when it is not empty
    if((xmin <= xmax) && (ymin <= ymax))
    {
        {
            auto fn = html_renderer->str_fmt("%s/bg%x.%s", (param.embed_image ? param.tmp_dir : param.dest_dir).c_str(), pageno, param.bg_format.c_str());
            if(param.embed_image)
                html_renderer->tmp_files.add((char*)fn);

            dump_image((char*)fn, xmin, ymin, xmax, ymax);
        }

        double h_scale = html_renderer->text_zoom_factor() * DEFAULT_DPI / param.h_dpi;
        double v_scale = html_renderer->text_zoom_factor() * DEFAULT_DPI / param.v_dpi;

        auto & f_page = *(html_renderer->f_curpage);
        auto & all_manager = html_renderer->all_manager;
        
        f_page << "<img class=\"" << CSS::BACKGROUND_IMAGE_CN 
            << " " << CSS::LEFT_CN      << all_manager.left.install(((double)xmin) * h_scale)
            << " " << CSS::BOTTOM_CN    << all_manager.bottom.install(((double)getBitmapHeight() - 1 - ymax) * v_scale)
            << " " << CSS::WIDTH_CN     << all_manager.width.install(((double)(xmax - xmin + 1)) * h_scale)
            << " " << CSS::HEIGHT_CN    << all_manager.height.install(((double)(ymax - ymin + 1)) * v_scale)
            << "\" alt=\"\" src=\"";

        if(param.embed_image)
        {
            auto path = html_renderer->str_fmt("%s/bg%x.%s", param.tmp_dir.c_str(), pageno, param.bg_format.c_str());
            ifstream fin((char*)path, ifstream::binary);
            if(!fin)
                throw string("Cannot read background image ") + (char*)path;

            auto iter = FORMAT_MIME_TYPE_MAP.find(param.bg_format);
            if(iter == FORMAT_MIME_TYPE_MAP.end())
                throw string("Image format not supported: ") + param.bg_format;

            string mime_type = iter->second;
            f_page << "data:" << mime_type << ";base64," << Base64Stream(fin);
        }
        else
        {
            f_page << (char*)html_renderer->str_fmt("bg%x.png", pageno);
        }
        f_page << "\"/>";
    }
}

// There might be mem leak when exception is thrown !
void SplashBackgroundRenderer::dump_image(const char * filename, int x1, int y1, int x2, int y2)
{
    int width = x2 - x1 + 1;
    int height = y2 - y1 + 1;
    if((width <= 0) || (height <= 0))
        throw "Bad metric for background image";

    FILE * f = fopen(filename, "wb");
    if(!f)
        throw string("Cannot open file for background image " ) + filename;

    // use unique_ptr to auto delete the object upon exception
    unique_ptr<ImgWriter> writer;

    if(false) { }
#ifdef ENABLE_LIBPNG
    else if(param.bg_format == "png")
    {
        writer = unique_ptr<ImgWriter>(new PNGWriter);
    }
#endif
#ifdef ENABLE_LIBJPEG
    else if(param.bg_format == "jpg")
    {
        writer = unique_ptr<ImgWriter>(new JpegWriter);
    }
#endif
    else
    {
        throw string("Image format not supported: ") + param.bg_format;
    }

    if(!writer->init(f, width, height, param.h_dpi, param.v_dpi))
        throw "Cannot initialize PNGWriter";
        
    auto * bitmap = getBitmap();
    assert(bitmap->getMode() == splashModeRGB8);

    SplashColorPtr data = bitmap->getDataPtr();
    int row_size = bitmap->getRowSize();

    vector<unsigned char*> pointers;
    pointers.reserve(height);
    SplashColorPtr p = data + y1 * row_size + x1 * 3;
    for(int i = 0; i < height; ++i)
    {
        pointers.push_back(p);
        p += row_size;
    }
    
    if(!writer->writePointers(pointers.data(), height)) {
        throw "Cannot write background image";
    }

    fclose(f);
}

} // namespace pdf2htmlEX
