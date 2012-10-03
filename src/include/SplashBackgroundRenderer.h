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

#include "HTMLRenderer.h"
#include "Param.h"

namespace pdf2htmlEX {

// Based on BackgroundRenderer from poppler
class SplashBackgroundRenderer : public SplashOutputDev 
{
public:
  static const SplashColor white;

  SplashBackgroundRenderer(HTMLRenderer * html_renderer, const Param * param)
      : SplashOutputDev(splashModeRGB8, 4, gFalse, (SplashColorPtr)&white, gTrue, gTrue)
      , html_renderer(html_renderer)
      , param(param)
  { }

  virtual ~SplashBackgroundRenderer() { }
  
  virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);

  virtual void stroke(GfxState *state) {
      if(!html_renderer->can_stroke(state))
          SplashOutputDev::stroke(state);
  }

  virtual void fill(GfxState *state) { 
      if(!html_renderer->can_fill(state))
          SplashOutputDev::fill(state);
  }

  void render_page(PDFDoc * doc, int pageno, const std::string & filename);

protected:
  HTMLRenderer * html_renderer;
  const Param * param;
};

} // namespace pdf2htmlEX

#endif // SPLASH_BACKGROUND_RENDERER_H__
