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

    auto bitmap = getBitmap();

    FILE *f;
    if(!(f = fopen(filename, "wb")))
        throw string("Cannot open file for writing: ") + filename;

    auto * writer = new PNGWriter();
    auto width = bitmap->getWidth();
    auto height = bitmap->getHeight();

    if(!writer->init(f, width, height,
               param->h_dpi, param->v_dpi))
        throw string("Cannot initialize PNGWriter"); 

    vector<unsigned char*> ptrs(height);
    {
        SplashColorPtr cur_row = bitmap->getDataPtr();
        auto row_size = bitmap->getRowSize();
        for(auto iter = ptrs.begin(); iter != ptrs.end(); ++iter)
        {
            *iter = cur_row;
            cur_row += row_size;
        }
    }

    if(!writer->writePointers(&ptrs.front(), height))
        throw string("Cannot write image");


    delete writer;

    fclose(f);
}

} // namespace pdf2htmlEX
