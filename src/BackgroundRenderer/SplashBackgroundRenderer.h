/*
 * Splash Background renderer
 * Render all those things not supported as Image, with Splash
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef SPLASH_BACKGROUND_RENDERER_H__
#define SPLASH_BACKGROUND_RENDERER_H__

#include <string>

#include <splash/SplashBitmap.h>
#include <SplashOutputDev.h>

#include "pdf2htmlEX-config.h"

#include "Param.h"
#include "HTMLRenderer/HTMLRenderer.h"

namespace pdf2htmlEX {

// Based on BackgroundRenderer from poppler
class SplashBackgroundRenderer : public BackgroundRenderer, SplashOutputDev 
{
public:
  static const SplashColor white;
  //format: "png" or "jpg", or "" for a default format
  SplashBackgroundRenderer(const std::string & format, HTMLRenderer * html_renderer, const Param & param);

  virtual ~SplashBackgroundRenderer() { }

  virtual void init(PDFDoc * doc);
  virtual bool render_page(PDFDoc * doc, int pageno);
  virtual void embed_image(int pageno);

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  virtual GBool interpretType3Chars() { return !param.process_type3; }

  virtual void startPage(int pageNum, GfxState *state, XRef *xrefA);
  
  virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);

  //for proof
  void beginTextObject(GfxState *state);
  void beginString(GfxState *state, GooString * str);
  void endTextObject(GfxState *state);
  void updateRender(GfxState *state);

protected:
  void dump_image(const char * filename, int x1, int y1, int x2, int y2);
  HTMLRenderer * html_renderer;
  const Param & param;
  std::string format;
  int drawn_char_count;
};

} // namespace pdf2htmlEX

#endif // SPLASH_BACKGROUND_RENDERER_H__
