/*
 * CairoBackgroundRenderer.cc
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */

#include "pdf2htmlEX-config.h"

#if HAVE_CAIRO

#include "CairoBackgroundRenderer.h"

namespace pdf2htmlEX {

void CairoBackgroundRenderer::drawChar(GfxState *state, double x, double y,
        double dx, double dy,
        double originX, double originY,
        CharCode code, int nBytes, Unicode *u, int uLen)
{
    //    CairoOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code, nBytes, u, uLen);
}

} // namespace pdf2htmlEX

#endif // HAVE_CAIRO

