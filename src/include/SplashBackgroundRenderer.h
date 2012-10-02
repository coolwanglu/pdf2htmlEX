/*
 * Splash Background renderer
 * Render all those things not supported as Image, with Splash
 *
 * by WangLu
 * 2012.08.06
 */


#ifndef BACKGROUND_RENDERER_H__
#define BACKGROUND_RENDERER_H__

#include <splash/SplashBitmap.h>
#include <SplashOutputDev.h>

namespace pdf2htmlEX {

// Based on BackgroundRenderer from poppler
class SplashBackgroundRenderer : public SplashOutputDev 
{
public:
  SplashBackgroundRenderer()
  { 
      SplashColor color;
      color[0] = color[1] = color[2] = 255;
      SplashOutputDev(splashModeRGB8, 4, gFlase, color, gTrue, gTrue)`
  }
  virtual ~BackgroundRenderer() { }
  
  virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);
};

}

#endif //BACKGROUND_RENDERER_H__
