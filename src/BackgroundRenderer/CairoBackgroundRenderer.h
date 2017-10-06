/*
 * Cairo Background renderer
 * Render all those things not supported as Image, with Cairo
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef CAIRO_BACKGROUND_RENDERER_H__
#define CAIRO_BACKGROUND_RENDERER_H__

#include <CairoOutputDev.h>
#include <cairo.h>
#include <cairo-svg.h>
#include <unordered_map>
#include <vector>
#include <string>

#include "pdf2htmlEX-config.h"

#include "Param.h"
#include "HTMLRenderer/HTMLRenderer.h"

namespace pdf2htmlEX {

// Based on BackgroundRenderer from poppler
class CairoBackgroundRenderer : public BackgroundRenderer, CairoOutputDev 
{
public:
  CairoBackgroundRenderer(HTMLRenderer * html_renderer, const Param & param);

  virtual ~CairoBackgroundRenderer();

  virtual void init(PDFDoc * doc);
  virtual bool render_page(PDFDoc * doc, int pageno);
  virtual void embed_image(int pageno);

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  virtual GBool interpretType3Chars() { return !param.process_type3; }

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
  virtual void setMimeData(GfxState *state, Stream *str, Object *ref, GfxImageColorMap *colorMap, cairo_surface_t *image);

protected:
  HTMLRenderer * html_renderer;
  const Param & param;
  cairo_surface_t * surface;

private:
  // convert bitmap stream id to bitmap file name. No pageno prefix,
  // because a bitmap may be shared by multiple pages.
  std::string build_bitmap_path(int id);
  // map<id_of_bitmap_stream, usage_count_in_all_svgs>
  // note: if a svg bg fallbacks to bitmap bg, its bitmaps are not taken into account.
  std::unordered_map<int, int> bitmaps_ref_count;
  // id of bitmaps' stream used by current page
  std::vector<int> bitmaps_in_current_page;
  int drawn_char_count;
};

}

#endif //CAIRO_BACKGROUND_RENDERER_H__
