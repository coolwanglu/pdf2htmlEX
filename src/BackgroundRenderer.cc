/*
 * BackgroundRenderer.cc
 *
 * Copyright (C) 2012 by Lu Wang coolwanglu<at>gmail.com
 */

#include <algorithm>

#include "GfxFont.h"

#include "BackgroundRenderer.h"
#include "util.h"

using std::all_of;

void BackgroundRenderer::drawChar(GfxState *state, double x, double y,
  double dx, double dy,
  double originX, double originY,
  CharCode code, int nBytes, Unicode *u, int uLen)
{
    auto font = state->getFont();
    /*
    if((font->getType() == fontType3) 
            || (font->getWMode()) 
            || (uLen == 0) 
            || (!all_of(u, u+uLen, isLegalUnicode))
      )
      */
    {
        SplashOutputDev::drawChar(state, x, y, dx, dy, originX, originY, code, nBytes, u, uLen);
    }
}

