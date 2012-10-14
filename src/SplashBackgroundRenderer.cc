/*
 * SplashBackgroundRenderer.cc
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */

#include <PDFDoc.h>

#include "SplashBackgroundRenderer.h"
#include "HTMLRenderer.h"

namespace pdf2htmlEX {

using std::string;

const SplashColor SplashBackgroundRenderer::white = {255,255,255};

void SplashBackgroundRenderer::drawChar(GfxState *state, double x, double y,
  double dx, double dy,
  double originX, double originY,
  CharCode code, int nBytes, Unicode *u, int uLen)
{
    if((state->getRender() & 3) == 3)
    {
        SplashOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code, nBytes, u, uLen);
    }
}

void SplashBackgroundRenderer::stroke(GfxState *state) 
{
  if(!html_renderer->can_stroke(state))
      SplashOutputDev::stroke(state);
}

void SplashBackgroundRenderer::fill(GfxState *state) 
{ 
  if(!html_renderer->can_fill(state))
      SplashOutputDev::fill(state);
}

static GBool annot_cb(Annot *, void *) {
    return false;
};

void SplashBackgroundRenderer::render_page(PDFDoc * doc, int pageno, const string & filename)
{
    doc->displayPage(this, pageno, param->h_dpi, param->v_dpi,
            0, true, false, false,
            nullptr, nullptr, &annot_cb, nullptr);

    getBitmap()->writeImgFile(splashFormatPng, 
            (char*)filename.c_str(),
            param->h_dpi, param->v_dpi);
}

} // namespace pdf2htmlEX
