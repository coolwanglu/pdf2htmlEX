/*
 * SplashBackgroundRenderer.cc
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */

#include <PDFDoc.h>

#include "SplashBackgroundRenderer.h"

namespace pdf2htmlEX {

using std::string;

const SplashColor SplashBackgroundRenderer::white = {255,255,255};

void SplashBackgroundRenderer::drawChar(GfxState *state, double x, double y,
  double dx, double dy,
  double originX, double originY,
  CharCode code, int nBytes, Unicode *u, int uLen)
{
    // draw characters as image when
    // - there is special filling method
    // - OR using a writing mode font
    // - OR using a Type 3 font
    if( ((state->getRender() & 3) == 3)
        || ( (state->getFont()) 
             && ( (state->getFont()->getWMode())
                  || (state->getFont()->getType() == fontType3)
                )
           )
      )
    {
        SplashOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code, nBytes, u, uLen);
    }
}

static GBool annot_cb(Annot *, void *) {
    return false;
};

void SplashBackgroundRenderer::render_page(PDFDoc * doc, int pageno, const string & filename)
{
    doc->displayPage(this, pageno, param->h_dpi, param->v_dpi,
            0, 
            (param->use_cropbox == 0), 
            false, false,
            nullptr, nullptr, &annot_cb, nullptr);

    getBitmap()->writeImgFile(splashFormatPng, 
            (char*)filename.c_str(),
            param->h_dpi, param->v_dpi);
}

} // namespace pdf2htmlEX
