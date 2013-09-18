/*
 * Cairo Background renderer
 * Render all those things not supported as Image, with Cairo
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef CAIRO_BACKGROUND_RENDERER_H__
#define CAIRO_BACKGROUND_RENDERER_H__

#include <BackgroundRenderer/CairoOutputDev/CairoOutputDev.h>
#include <cairo.h>
#include <cairo-svg.h>

#include "pdf2htmlEX-config.h"

#include "Param.h"
#include "HTMLRenderer/HTMLRenderer.h"

namespace pdf2htmlEX {

// Based on BackgroundRenderer from poppler
class CairoBackgroundRenderer : public BackgroundRenderer, CairoOutputDev 
{
public:
  CairoBackgroundRenderer(HTMLRenderer * html_renderer, const Param & param)
      : CairoOutputDev()
      , html_renderer(html_renderer)
      , param(param)
      , surface(nullptr)
  { }

  virtual ~CairoBackgroundRenderer() { }

  virtual void init(PDFDoc * doc);
  virtual void render_page(PDFDoc * doc, int pageno);
  virtual void embed_image(int pageno);

  virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);

protected:
  HTMLRenderer * html_renderer;
  const Param & param;
  cairo_surface_t * surface;
};

}

#endif //CAIRO_BACKGROUND_RENDERER_H__
