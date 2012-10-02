/*
 * Splash Background renderer
 * Render all those things not supported as Image, with Splash
 *
 * by WangLu
 * 2012.08.06
 */


#ifndef SPLASH_BACKGROUND_RENDERER_H__
#define SPLASH_BACKGROUND_RENDERER_H__

#include <string>

#include <splash/SplashBitmap.h>
#include <SplashOutputDev.h>

#include "Param.h"

namespace pdf2htmlEX {

// Based on BackgroundRenderer from poppler
class SplashBackgroundRenderer : public SplashOutputDev 
{
public:
  static const SplashColor white;

  SplashBackgroundRenderer(const Param * param)
      : SplashOutputDev(splashModeRGB8, 4, gFalse, (SplashColorPtr)&white, gTrue, gTrue)
      , param(param)
  { }

  virtual ~SplashBackgroundRenderer() { }
  
  virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);

  void render_page(PDFDoc * doc, int pageno, const std::string & filename);

protected:
  const Param * param;
};

} // namespace pdf2htmlEX

#endif // SPLASH_BACKGROUND_RENDERER_H__
