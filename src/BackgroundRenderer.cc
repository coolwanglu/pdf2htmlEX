/*
 * BackgroundRenderer.cc
 *
 * Copyright (C) 2012 by Lu Wang coolwanglu<at>gmail.com
 */

#include "GfxFont.h"

#include "BackgroundRenderer.h"

void BackgroundRenderer::drawChar(GfxState *state, double x, double y,
  double dx, double dy,
  double originX, double originY,
  CharCode code, int nBytes, Unicode *u, int uLen)
{
    auto font = state->getFont();
//    if((font->getType() == fontType3) || (font->getWMode()))
    {
        SplashOutputDev::drawChar(state, x, y, dx, dy, originX, originY, code, nBytes, u, uLen);
    }
}

