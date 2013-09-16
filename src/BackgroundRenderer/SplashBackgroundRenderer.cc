/*
 * SplashBackgroundRenderer.cc
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */

#include <fstream>

#include <PDFDoc.h>

#include "Base64Stream.h"

#include "SplashBackgroundRenderer.h"

namespace pdf2htmlEX {

using std::string;
using std::ifstream;

const SplashColor SplashBackgroundRenderer::white = {255,255,255};

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
    // - OR using a Type 3 font
    if((param.fallback)
       || ( (state->getFont()) 
            && ( (state->getFont()->getWMode())
                 || (state->getFont()->getType() == fontType3)
               )
          )
      )
    {
        SplashOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code,nBytes,u,uLen);
    }
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
    int xmin, xmax, ymin, ymax;
    getModRegion(&xmin, &ymin, &xmax, &ymax);

    // dump the background image only when it is not empty
    if((xmin <= xmax) && (ymin <= ymax))
    {
        {
            auto fn = html_renderer->str_fmt("%s/bg%x.png", (param.embed_image ? param.tmp_dir : param.dest_dir).c_str(), pageno);
            if(param.embed_image)
                html_renderer->tmp_files.add((char*)fn);

            getBitmap()->writeImgFile(splashFormatPng, 
                    (char*)fn,
                    param.h_dpi, param.v_dpi);
        }

        auto & f_page = *(html_renderer->f_curpage);
        f_page << "<img class=\"" << CSS::BACKGROUND_IMAGE_CN 
            << "\" alt=\"\" src=\"";
        if(param.embed_image)
        {
            auto path = html_renderer->str_fmt("%s/bg%x.png", param.tmp_dir.c_str(), pageno);
            ifstream fin((char*)path, ifstream::binary);
            if(!fin)
                throw string("Cannot read background image ") + (char*)path;
            f_page << "data:image/png;base64," << Base64Stream(fin);
        }
        else
        {
            f_page << (char*)html_renderer->str_fmt("bg%x.png", pageno);
        }
        f_page << "\"/>";
    }
}

} // namespace pdf2htmlEX
