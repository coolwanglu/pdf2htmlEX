/*
 * SplashBackgroundRenderer.cc
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */

#include <string>
#include <vector>

#include <PDFDoc.h>
#include <goo/PNGWriter.h>

#include "SplashBackgroundRenderer.h"
#include "HTMLRenderer.h"


namespace pdf2htmlEX {

using std::string;
using std::vector;

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

void SplashBackgroundRenderer::dump_to(const char * filename)
{
    /*
     * HTMLRenderer will handle upsideDown()
     * so we need to flip it here
     */
#ifndef ENABLE_LIBPNG
    throw string("poppler was not built with libpng!");
#endif

    FILE *f = fopen(filename, "wb");
    if(!f)
        throw string("Cannot open file for writing: ") + filename;

    getBitmap()->writeImgFile(splashFormatPng, f, param->h_dpi, param->v_dpi);

    fclose(f);
}

} // namespace pdf2htmlEX
