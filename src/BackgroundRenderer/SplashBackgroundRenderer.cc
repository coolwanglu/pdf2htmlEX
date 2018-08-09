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
#include <goo/ImgWriter.h>
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

SplashBackgroundRenderer::SplashBackgroundRenderer(const string & imgFormat, HTMLRenderer * html_renderer, const Param & param)
    : SplashOutputDev(splashModeRGB8, 4, gFalse, (SplashColorPtr)(&white))
    , html_renderer(html_renderer)
    , param(param)
    , format(imgFormat)
{
    bool supported = false;
#ifdef ENABLE_LIBPNG
    if (format.empty())
        format = "png";
    supported = supported || format == "png";
#endif
#ifdef ENABLE_LIBJPEG
    if (format.empty())
        format = "jpg";
    supported = supported || format == "jpg";
#endif
    if (!supported)
    {
        throw string("Image format not supported: ") + format;
    }
}

/*
 * SplashOutputDev::startPage would paint the whole page with the background color
 * And thus have modified region set to the whole page area
 * We do not want that.
 */
void SplashBackgroundRenderer::startPage(int pageNum, GfxState *state, XRef *xrefA)
{
    SplashOutputDev::startPage(pageNum, state, xrefA);
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
    // - OR the text is used as path
    if((param.fallback || param.proof)
       || ( (state->getFont()) 
            && ( (state->getFont()->getWMode())
                 || ((state->getFont()->getType() == fontType3) && (!param.process_type3))
                 || (state->getRender() >= 4)
               )
          )
      )
    {
        SplashOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code,nBytes,u,uLen);
    }
    // If a char is treated as image, it is not subject to cover test
    // (see HTMLRenderer::drawString), so don't increase drawn_char_count.
    else if (param.correct_text_visibility) {
        if (html_renderer->is_char_covered(drawn_char_count))
            SplashOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code,nBytes,u,uLen);
        drawn_char_count++;
    }
}

void SplashBackgroundRenderer::beginTextObject(GfxState *state)
{
    if (param.proof == 2)
        proof_begin_text_object(state, this);
    SplashOutputDev::beginTextObject(state);
}

void SplashBackgroundRenderer::beginString(GfxState *state, GooString * str)
{
    if (param.proof == 2)
        proof_begin_string(state, this);
    SplashOutputDev::beginString(state, str);
}

void SplashBackgroundRenderer::endTextObject(GfxState *state)
{
    if (param.proof == 2)
        proof_end_text_object(state, this);
    SplashOutputDev::endTextObject(state);
}

void SplashBackgroundRenderer::updateRender(GfxState *state)
{
    if (param.proof == 2)
        proof_update_render(state, this);
    SplashOutputDev::updateRender(state);
}

void SplashBackgroundRenderer::init(PDFDoc * doc)
{
    startDoc(doc);
}

static GBool annot_cb(Annot *, void * pflag) {
    return (*((bool*)pflag)) ? gTrue : gFalse;
};

bool SplashBackgroundRenderer::render_page(PDFDoc * doc, int pageno)
{
    drawn_char_count = 0;
    bool process_annotation = param.process_annotation;
    doc->displayPage(this, pageno, param.h_dpi, param.v_dpi,
            0, 
            (!(param.use_cropbox)),
            false, false,
            nullptr, nullptr, &annot_cb, &process_annotation);
    return true;
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
            auto fn = html_renderer->str_fmt("%s/bg%x.%s", (param.embed_image ? param.tmp_dir : param.dest_dir).c_str(), pageno, format.c_str());
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
            auto path = html_renderer->str_fmt("%s/bg%x.%s", param.tmp_dir.c_str(), pageno, format.c_str());
            ifstream fin((char*)path, ifstream::binary);
            if(!fin)
                throw string("Cannot read background image ") + (char*)path;

            auto iter = FORMAT_MIME_TYPE_MAP.find(format);
            if(iter == FORMAT_MIME_TYPE_MAP.end())
                throw string("Image format not supported: ") + format;

            string mime_type = iter->second;
            f_page << "data:" << mime_type << ";base64," << Base64Stream(fin);
        }
        else
        {
            f_page << (char*)html_renderer->str_fmt("bg%x.%s", pageno, format.c_str());
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
    else if(format == "png")
    {
        writer = unique_ptr<ImgWriter>(new PNGWriter);
    }
#endif
#ifdef ENABLE_LIBJPEG
    else if(format == "jpg")
    {
        writer = unique_ptr<ImgWriter>(new JpegWriter);
    }
#endif
    else
    {
        throw string("Image format not supported: ") + format;
    }

    if(!writer->init(f, width, height, param.h_dpi, param.v_dpi))
        throw "Cannot initialize image writer";
        
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
    
    if(!writer->writePointers(pointers.data(), height)) 
    {
        throw "Cannot write background image";
    }

    if(!writer->close())
    {
        throw "Cannot finish background image";
    }

    fclose(f);
}

} // namespace pdf2htmlEX
