/*
 * BackgroundRenderer.cc
 *
 * Copyright (C) 2012 by Lu Wang coolwanglu<at>gmail.com
 */

#include <algorithm>

#include "GfxFont.h"

#include "BackgroundRenderer.h"
#include "util.h"

using namespace pdf2htmlEX;

void BackgroundRenderer::drawChar(GfxState *state, double x, double y,
  double dx, double dy,
  double originX, double originY,
  CharCode code, int nBytes, Unicode *u, int uLen)
{
//SplashOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code, nBytes, u, uLen);
}

