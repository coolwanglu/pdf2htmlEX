//========================================================================
//
// CairoOutputDev.h
//
// Copyright 2003 Glyph & Cog, LLC
// Copyright 2004 Red Hat, INC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005-2008 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2005, 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2005 Nickolay V. Shmyrev <nshmyrev@yandex.ru>
// Copyright (C) 2006-2011, 2013 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2008, 2009, 2011-2016 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2008 Michael Vrable <mvrable@cs.ucsd.edu>
// Copyright (C) 2010-2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2015 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2016 Jason Crain <jason@aquaticape.us>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef CAIROOUTPUTDEV_H
#define CAIROOUTPUTDEV_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include <cairo-ft.h>
#include "OutputDev.h"
#include "TextOutputDev.h"
#include "GfxState.h"

class PDFDoc;
class GfxState;
class GfxPath;
class Gfx8BitFont;
struct GfxRGB;
class CairoFontEngine;
class CairoFont;

//------------------------------------------------------------------------

//------------------------------------------------------------------------
// CairoImage
//------------------------------------------------------------------------
class CairoImage {
public:
  // Constructor.
  CairoImage (double x1, double y1, double x2, double y2);

  // Destructor.
  ~CairoImage ();

  // Set the image cairo surface
  void setImage (cairo_surface_t *image);
  
  // Get the image cairo surface
  cairo_surface_t *getImage () const { return image; }

  // Get the image rectangle
  void getRect (double *xa1, double *ya1, double *xa2, double *ya2)
	  { *xa1 = x1; *ya1 = y1; *xa2 = x2; *ya2 = y2; }
  
private:
  cairo_surface_t *image;  // image cairo surface
  double x1, y1;          // upper left corner
  double x2, y2;          // lower right corner
};


//------------------------------------------------------------------------
// CairoOutputDev
//------------------------------------------------------------------------

class CairoOutputDev: public OutputDev {
public:

  // Constructor.
  CairoOutputDev();

  // Destructor.
  virtual ~CairoOutputDev();

  //----- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  GBool upsideDown() override { return gTrue; }

  // Does this device use drawChar() or drawString()?
  GBool useDrawChar() override { return gTrue; }

  // Does this device use tilingPatternFill()?  If this returns false,
  // tiling pattern fills will be reduced to a series of other drawing
  // operations.
  GBool useTilingPatternFill() override { return gTrue; }

  // Does this device use functionShadedFill(), axialShadedFill(), and
  // radialShadedFill()?  If this returns false, these shaded fills
  // will be reduced to a series of other drawing operations.
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 12, 0)
  GBool useShadedFills(int type) override { return type <= 7; }
#else
  GBool useShadedFills(int type) override { return type > 1 && type < 4; }
#endif

  // Does this device use FillColorStop()?
  GBool useFillColorStop() override { return gTrue; }

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  GBool interpretType3Chars() override { return gFalse; }

  // Does this device need to clip pages to the crop box even when the
  // box is the crop box?
  GBool needClipToCropBox() override { return gTrue; }

  //----- initialization and control

  // Start a page.
  void startPage(int pageNum, GfxState *state, XRef *xref) override;

  // End a page.
  void endPage() override;

  //----- save/restore graphics state
  void saveState(GfxState *state) override;
  void restoreState(GfxState *state) override;

  //----- update graphics state
  void updateAll(GfxState *state) override;
  void setDefaultCTM(double *ctm) override;
  void updateCTM(GfxState *state, double m11, double m12,
		 double m21, double m22, double m31, double m32) override;
  void updateLineDash(GfxState *state) override;
  void updateFlatness(GfxState *state) override;
  void updateLineJoin(GfxState *state) override;
  void updateLineCap(GfxState *state) override;
  void updateMiterLimit(GfxState *state) override;
  void updateLineWidth(GfxState *state) override;
  void updateFillColor(GfxState *state) override;
  void updateStrokeColor(GfxState *state) override;
  void updateFillOpacity(GfxState *state) override;
  void updateStrokeOpacity(GfxState *state) override;
  void updateFillColorStop(GfxState *state, double offset) override;
  void updateBlendMode(GfxState *state) override;

  //----- update text state
  void updateFont(GfxState *state) override;

  //----- path painting
  void stroke(GfxState *state) override;
  void fill(GfxState *state) override;
  void eoFill(GfxState *state) override;
  void clipToStrokePath(GfxState *state) override;
  GBool tilingPatternFill(GfxState *state, Gfx *gfx, Catalog *cat, Object *str,
			  double *pmat, int paintType, int tilingType, Dict *resDict,
			  double *mat, double *bbox,
			  int x0, int y0, int x1, int y1,
			  double xStep, double yStep) override;
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 12, 0)
  GBool functionShadedFill(GfxState *state, GfxFunctionShading *shading) override;
#endif
  GBool axialShadedFill(GfxState *state, GfxAxialShading *shading, double tMin, double tMax) override;
  GBool axialShadedSupportExtend(GfxState *state, GfxAxialShading *shading) override;
  GBool radialShadedFill(GfxState *state, GfxRadialShading *shading, double sMin, double sMax) override;
  GBool radialShadedSupportExtend(GfxState *state, GfxRadialShading *shading) override;
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 12, 0)
  GBool gouraudTriangleShadedFill(GfxState *state, GfxGouraudTriangleShading *shading) override;
  GBool patchMeshShadedFill(GfxState *state, GfxPatchMeshShading *shading) override;
#endif

  //----- path clipping
  void clip(GfxState *state) override;
  void eoClip(GfxState *state) override;

  //----- text drawing
  void beginString(GfxState *state, GooString *s) override;
  void endString(GfxState *state) override;
  void drawChar(GfxState *state, double x, double y,
		double dx, double dy,
		double originX, double originY,
		CharCode code, int nBytes, Unicode *u, int uLen) override;
  void beginActualText(GfxState *state, GooString *text) override;
  void endActualText(GfxState *state) override;

  GBool beginType3Char(GfxState *state, double x, double y,
		       double dx, double dy,
		       CharCode code, Unicode *u, int uLen) override;
  void endType3Char(GfxState *state) override;
  void beginTextObject(GfxState *state) override;
  void endTextObject(GfxState *state) override;

  //----- image drawing
  void drawImageMask(GfxState *state, Object *ref, Stream *str,
		     int width, int height, GBool invert, GBool interpolate,
		     GBool inlineImg) override;
  void setSoftMaskFromImageMask(GfxState *state,
				Object *ref, Stream *str,
				int width, int height, GBool invert,
				GBool inlineImg, double *baseMatrix) override;
  void unsetSoftMaskFromImageMask(GfxState *state, double *baseMatrix) override;
  void drawImageMaskPrescaled(GfxState *state, Object *ref, Stream *str,
			      int width, int height, GBool invert, GBool interpolate,
			      GBool inlineImg);
  void drawImageMaskRegular(GfxState *state, Object *ref, Stream *str,
			    int width, int height, GBool invert, GBool interpolate,
			    GBool inlineImg);

  void drawImage(GfxState *state, Object *ref, Stream *str,
		 int width, int height, GfxImageColorMap *colorMap,
		 GBool interpolate, int *maskColors, GBool inlineImg) override;
  void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
			   int width, int height,
			   GfxImageColorMap *colorMap,
			   GBool interpolate,
			   Stream *maskStr,
			   int maskWidth, int maskHeight,
			   GfxImageColorMap *maskColorMap,
			   GBool maskInterpolate) override;

  void drawMaskedImage(GfxState *state, Object *ref, Stream *str,
		       int width, int height,
		       GfxImageColorMap *colorMap,
		       GBool interpolate,
		       Stream *maskStr,
		       int maskWidth, int maskHeight,
		       GBool maskInvert, GBool maskInterpolate) override;

  //----- transparency groups and soft masks
  void beginTransparencyGroup(GfxState * /*state*/, double * /*bbox*/,
                                      GfxColorSpace * /*blendingColorSpace*/,
                                      GBool /*isolated*/, GBool /*knockout*/,
                                      GBool /*forSoftMask*/) override;
  void endTransparencyGroup(GfxState * /*state*/) override;
  void popTransparencyGroup();
  void paintTransparencyGroup(GfxState * /*state*/, double * /*bbox*/) override;
  void setSoftMask(GfxState * /*state*/, double * /*bbox*/, GBool /*alpha*/,
		   Function * /*transferFunc*/, GfxColor * /*backdropColor*/) override;
  void clearSoftMask(GfxState * /*state*/) override;

  //----- Type 3 font operators
  void type3D0(GfxState *state, double wx, double wy) override;
  void type3D1(GfxState *state, double wx, double wy,
	       double llx, double lly, double urx, double ury) override;

  //----- special access
  
  // Called to indicate that a new PDF document has been loaded.
  void startDoc(PDFDoc *docA, CairoFontEngine *fontEngine = NULL);
 
  GBool isReverseVideo() { return gFalse; }
  
  void setCairo (cairo_t *cr);
  void setTextPage (TextPage *text);
  void setPrinting (GBool printing) { this->printing = printing; needFontUpdate = gTrue; }
  void setAntialias(cairo_antialias_t antialias);

  void setInType3Char(GBool inType3Char) { this->inType3Char = inType3Char; }
  void getType3GlyphWidth (double *wx, double *wy) { *wx = t3_glyph_wx; *wy = t3_glyph_wy; }
  GBool hasType3GlyphBBox () { return t3_glyph_has_bbox; }
  double *getType3GlyphBBox () { return t3_glyph_bbox; }

protected:
  void doPath(cairo_t *cairo, GfxState *state, GfxPath *path);
  cairo_surface_t *downscaleSurface(cairo_surface_t *orig_surface);
  void getScaledSize(const cairo_matrix_t *matrix,
                     int orig_width, int orig_height,
		     int *scaledWidth, int *scaledHeight);
  cairo_filter_t getFilterForSurface(cairo_surface_t *image,
				     GBool interpolate);
  GBool getStreamData (Stream *str, char **buffer, int *length);
  void setMimeData(GfxState *state, Stream *str, Object *ref,
		   GfxImageColorMap *colorMap, cairo_surface_t *image);
  void fillToStrokePathClip(GfxState *state);
  void alignStrokeCoords(GfxSubpath *subpath, int i, double *x, double *y);
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 14, 0)
  GBool setMimeDataForJBIG2Globals (Stream *str, cairo_surface_t *image);
#endif
  static void setContextAntialias(cairo_t *cr, cairo_antialias_t antialias);

  GfxRGB fill_color, stroke_color;
  cairo_pattern_t *fill_pattern, *stroke_pattern;
  double fill_opacity;
  double stroke_opacity;
  GBool stroke_adjust;
  GBool adjusted_stroke_width;
  GBool align_stroke_coords;
  CairoFont *currentFont;
  XRef *xref;

  struct StrokePathClip {
    GfxPath *path;
    cairo_matrix_t ctm;
    double line_width;
    double *dashes;
    int dash_count;
    double dash_offset;
    cairo_line_cap_t cap;
    cairo_line_join_t join;
    double miter;
    int ref_count;
  } *strokePathClip;

  PDFDoc *doc;			// the current document

  static FT_Library ft_lib;
  static GBool ft_lib_initialized;

  CairoFontEngine *fontEngine;
  GBool fontEngine_owner;

  cairo_t *cairo;
  cairo_matrix_t orig_matrix;
  GBool needFontUpdate;                // set when the font needs to be updated
  GBool printing;
  GBool use_show_text_glyphs;
  GBool text_matrix_valid;
  cairo_surface_t *surface;
  cairo_glyph_t *glyphs;
  int glyphCount;
  cairo_text_cluster_t *clusters;
  int clusterCount;
  char *utf8;
  int utf8Count;
  int utf8Max;
  cairo_path_t *textClipPath;
  GBool inUncoloredPattern;     // inside a uncolored pattern (PaintType = 2)
  GBool inType3Char;		// inside a Type 3 CharProc
  double t3_glyph_wx, t3_glyph_wy;
  GBool t3_glyph_has_bbox;
  double t3_glyph_bbox[4];
  cairo_antialias_t antialias;
  GBool prescaleImages;

  TextPage *text;		// text for the current page
  ActualText *actualText;

  cairo_pattern_t *group;
  cairo_pattern_t *shape;
  cairo_pattern_t *mask;
  cairo_matrix_t mask_matrix;
  cairo_surface_t *cairo_shape_surface;
  cairo_t *cairo_shape;
  int knockoutCount;
  struct ColorSpaceStack {
    GBool knockout;
    GfxColorSpace *cs;
    cairo_matrix_t group_matrix;
    struct ColorSpaceStack *next;
  } * groupColorSpaceStack;

  struct MaskStack {
    cairo_pattern_t *mask;
    cairo_matrix_t mask_matrix;
    struct MaskStack *next;
  } *maskStack;

};

//------------------------------------------------------------------------
// CairoImageOutputDev
//------------------------------------------------------------------------

//XXX: this should ideally not inherit from CairoOutputDev but use it instead perhaps
class CairoImageOutputDev: public CairoOutputDev {
public:

  // Constructor.
  CairoImageOutputDev();

  // Destructor.
  virtual ~CairoImageOutputDev();

  //----- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  GBool upsideDown() override { return gTrue; }

  // Does this device use drawChar() or drawString()?
  GBool useDrawChar() override { return gFalse; }

  // Does this device use tilingPatternFill()?  If this returns false,
  // tiling pattern fills will be reduced to a series of other drawing
  // operations.
  GBool useTilingPatternFill() override { return gTrue; }

  // Does this device use functionShadedFill(), axialShadedFill(), and
  // radialShadedFill()?  If this returns false, these shaded fills
  // will be reduced to a series of other drawing operations.
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 11, 2)
  GBool useShadedFills(int type) override { return type <= 7; }
#else
  GBool useShadedFills(int type) override { return type < 4; }
#endif

  // Does this device use FillColorStop()?
  GBool useFillColorStop() override { return gFalse; }

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  GBool interpretType3Chars() override { return gFalse; }

  // Does this device need non-text content?
  GBool needNonText() override { return gTrue; }

  //----- save/restore graphics state
  void saveState(GfxState *state) override { }
  void restoreState(GfxState *state) override { }

  //----- update graphics state
  void updateAll(GfxState *state) override { }
  void setDefaultCTM(double *ctm) override { }
  void updateCTM(GfxState *state, double m11, double m12,
		 double m21, double m22, double m31, double m32) override { }
  void updateLineDash(GfxState *state) override { }
  void updateFlatness(GfxState *state) override { }
  void updateLineJoin(GfxState *state) override { }
  void updateLineCap(GfxState *state) override { }
  void updateMiterLimit(GfxState *state) override { }
  void updateLineWidth(GfxState *state) override { }
  void updateFillColor(GfxState *state) override { }
  void updateStrokeColor(GfxState *state) override { }
  void updateFillOpacity(GfxState *state) override { }
  void updateStrokeOpacity(GfxState *state) override { }
  void updateBlendMode(GfxState *state) override { }

  //----- update text state
  void updateFont(GfxState *state) override { }

  //----- path painting
  void stroke(GfxState *state) override { }
  void fill(GfxState *state) override { }
  void eoFill(GfxState *state) override { }
  void clipToStrokePath(GfxState *state) override { }
  GBool tilingPatternFill(GfxState *state, Gfx *gfx, Catalog *cat, Object *str,
			  double *pmat, int paintType, int tilingType, Dict *resDict,
			  double *mat, double *bbox,
			  int x0, int y0, int x1, int y1,
			  double xStep, double yStep) override { return gTrue; }
  GBool axialShadedFill(GfxState *state,
			GfxAxialShading *shading,
			double tMin, double tMax) override { return gTrue; }
  GBool radialShadedFill(GfxState *state,
			 GfxRadialShading *shading,
			 double sMin, double sMax) override { return gTrue; }

  //----- path clipping
  void clip(GfxState *state) override { }
  void eoClip(GfxState *state) override { }

  //----- image drawing
  void drawImageMask(GfxState *state, Object *ref, Stream *str,
		     int width, int height, GBool invert,
		     GBool interpolate, GBool inlineImg) override;
  void drawImage(GfxState *state, Object *ref, Stream *str,
		 int width, int height, GfxImageColorMap *colorMap,
		 GBool interpolate, int *maskColors, GBool inlineImg) override;
  void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
			   int width, int height,
			   GfxImageColorMap *colorMap,
			   GBool interpolate,
			   Stream *maskStr,
			   int maskWidth, int maskHeight,
			   GfxImageColorMap *maskColorMap,
			   GBool maskInterpolate) override;
  void drawMaskedImage(GfxState *state, Object *ref, Stream *str,
		       int width, int height,
		       GfxImageColorMap *colorMap,
		       GBool interpolate,
		       Stream *maskStr,
		       int maskWidth, int maskHeight,
		       GBool maskInvert, GBool maskInterpolate) override;
  void setSoftMaskFromImageMask(GfxState *state, Object *ref, Stream *str,
				int width, int height, GBool invert,
				GBool inlineImg, double *baseMatrix) override;
  void unsetSoftMaskFromImageMask(GfxState *state, double *baseMatrix) override {}


  //----- transparency groups and soft masks
  void beginTransparencyGroup(GfxState * /*state*/, double * /*bbox*/,
			      GfxColorSpace * /*blendingColorSpace*/,
			      GBool /*isolated*/, GBool /*knockout*/,
			      GBool /*forSoftMask*/) override {}
  void endTransparencyGroup(GfxState * /*state*/) override {}
  void paintTransparencyGroup(GfxState * /*state*/, double * /*bbox*/) override {}
  void setSoftMask(GfxState * /*state*/, double * /*bbox*/, GBool /*alpha*/,
		   Function * /*transferFunc*/, GfxColor * /*backdropColor*/) override {}
  void clearSoftMask(GfxState * /*state*/) override {}

  //----- Image list
  // By default images are not rendred
  void setImageDrawDecideCbk(GBool (*cbk)(int img_id, void *data),
			     void *data) { imgDrawCbk = cbk; imgDrawCbkData = data; }
  // Iterate through list of images.
  int getNumImages() const { return numImages; }
  CairoImage *getImage(int i) const { return images[i]; }

private:
  void saveImage(CairoImage *image);
  void getBBox(GfxState *state, int width, int height,
               double *x1, double *y1, double *x2, double *y2);
  
  CairoImage **images;
  int numImages;
  int size;
  GBool (*imgDrawCbk)(int img_id, void *data);
  void *imgDrawCbkData;
};

#endif