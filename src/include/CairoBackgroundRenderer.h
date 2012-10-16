/*
 * Cairo Background renderer
 * Render all those things not supported as Image, with Cairo
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef CAIRO_BACKGROUND_RENDERER_H__
#define CAIRO_BACKGROUND_RENDERER_H__

#include <ostream>

#include <cairo.h>

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
      , context(nullptr)
      , surface(nullptr)
      , out(nullptr)
  { }

  virtual ~CairoBackgroundRenderer(void);

  virtual void pre_process(PDFDoc * doc);
  
  void startPage(int pageNum, GfxState * state);
  virtual void endPage(void);

  virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);

  std::string get_cur_page_filename(void) const { return cur_page_filename; }
  void set_cur_page_filename(const std::string & fn) { cur_page_filename = fn; }
  void dump(void);

protected:
  HTMLRenderer * html_renderer;
  const Param * param;

  cairo_t * context;
  cairo_surface_t * surface;
  std::ostream * out;

  std::string cur_page_filename;
};

}

#endif //CAIRO_BACKGROUND_RENDERER_H__
