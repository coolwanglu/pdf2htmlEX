/*
 * Background renderer
 * Render all those things not supported as Image
 *
 * by WangLu
 * 2012.08.06
 */


#ifndef BACKGROUND_RENDERER_H__
#define BACKGROUND_RENDERER_H__

#include <SplashOutputDev.h>

// Based on BackgroundRenderer from poppler
class BackgroundRenderer : public SplashOutputDev {
public:
  BackgroundRenderer(SplashColorMode colorModeA, int bitmapRowPadA,
        GBool reverseVideoA, SplashColorPtr paperColorA,
        GBool bitmapTopDownA = gTrue,
        GBool allowAntialiasA = gTrue) : SplashOutputDev(colorModeA,
            bitmapRowPadA, reverseVideoA, paperColorA, bitmapTopDownA,
            allowAntialiasA) { }
  virtual ~BackgroundRenderer() { }
  
  void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);
};




#endif //BACKGROUND_RENDERER_H__
