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

void SplashBackgroundRenderer::dump_to(char * filename)
{
    getBitmap()->writeImgFile(splashFormatPng, 
            filename, param->h_dpi, param->v_dpi);
}

} // namespace pdf2htmlEX
