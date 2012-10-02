/*
 * Splash Background renderer
 * Render all those things not supported as Image, with Splash
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef SPLASH_BACKGROUND_RENDERER_H__
#define SPLASH_BACKGROUND_RENDERER_H__

#include <splash/SplashBitmap.h>
#include <SplashOutputDev.h>

namespace pdf2htmlEX {

// Based on BackgroundRenderer from poppler
class SplashBackgroundRenderer : public SplashOutputDev 
{
public:
  static const SplashColor white;

  SplashBackgroundRenderer()
      :SplashOutputDev(splashModeRGB8, 4, gFalse, (SplashColorPtr)&white, gTrue, gTrue)
  { }

  virtual ~SplashBackgroundRenderer() { }
  
  virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);
};

}

#endif //SPLASH_BACKGROUND_RENDERER_H__
