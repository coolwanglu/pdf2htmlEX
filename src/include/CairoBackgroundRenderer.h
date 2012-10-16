/*
 * Cairo Background renderer
 * Render all those things not supported as Image, with Cairo
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef CAIRO_BACKGROUND_RENDERER_H__
#define CAIRO_BACKGROUND_RENDERER_H__

#include <ostream>

#include <CairoOutputDev.h>

#include "Param.h"

namespace pdf2htmlEX {

class HTMLRenderer;

// Based on BackgroundRenderer from poppler
class CairoBackgroundRenderer : public CairoOutputDev 
{
public:
  CairoBackgroundRenderer(HTMLRenderer * html_renderer, const Param * param)
      :CairoOutputDev()
      , html_renderer(html_renderer)
      , param(param)
      , surface(nullptr)
      , out(nullptr)
  { }

  virtual ~CairoBackgroundRenderer() { }

  virtual void pre_process(PDFDoc * doc);
  
  virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);

  void render_page(PDFDoc * doc, int pageno, const std::string & filename);

  void dump_to(const char * filename) { }

protected:
  HTMLRenderer * html_renderer;
  const Param * param;

  cairo_suface_t * surface;
  std::ostream * out;
};

}

#endif //CAIRO_BACKGROUND_RENDERER_H__
