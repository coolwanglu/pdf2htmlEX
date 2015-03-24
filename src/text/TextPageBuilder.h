/*
 * TextPageBuilder.h
 *
 * Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef TEXTPAGEBUILDER_H__
#define TEXTPAGEBUILDER_H__

#include <OutputDev.h>
#include <GfxState.h>
#include <Stream.h>
#include <PDFDoc.h>

namespace pdf2htmlEX {

// Dump PDF instruction into nodes;
struct TextPageBuilder : OutputDev
{
    ////////////////////////////////////////////////////
    // OutputDev interface
    ////////////////////////////////////////////////////
    
    // Does this device use upside-down coordinates?
    // (Upside-down means (0,0) is the top left corner of the page.)
    virtual GBool upsideDown() { return gFalse; }

    // Does this device use drawChar() or drawString()?
    virtual GBool useDrawChar() { return gFalse; }

    // Does this device use functionShadedFill(), axialShadedFill(), and
    // radialShadedFill()?  If this returns false, these shaded fills
    // will be reduced to a series of other drawing operations.
    virtual GBool useShadedFills(int type) { return (type == 2) ? gTrue: gFalse; }

    // Does this device use beginType3Char/endType3Char?  Otherwise,
    // text in Type 3 fonts will be drawn with drawChar/drawString.
    virtual GBool interpretType3Chars() { return gFalse; }

    // Does this device need non-text content?
    virtual GBool needNonText() { return gTrue; }

    // Does this device need to clip pages to the crop box even when the
    // box is the crop box?
    virtual GBool needClipToCropBox() { return gTrue; }

    virtual void setDefaultCTM(double *ctm);

    virtual void startPage(int pageNum, GfxState *state, XRef * xref);
    virtual void endPage();

    virtual void restoreState(GfxState * state);
    virtual void saveState(GfxState *state);

    virtual void updateAll(GfxState * state);

    virtual void updateRise(GfxState * state);
    virtual void updateTextPos(GfxState * state);
    virtual void updateTextShift(GfxState * state, double shift);

    virtual void updateFont(GfxState * state);
    virtual void updateCTM(GfxState * state, double m11, double m12, double m21, double m22, double m31, double m32);
    virtual void updateTextMat(GfxState * state);
    virtual void updateHorizScaling(GfxState * state);

    virtual void updateCharSpace(GfxState * state);
    virtual void updateWordSpace(GfxState * state);

    virtual void updateRender(GfxState * state);

    virtual void updateFillColorSpace(GfxState * state);
    virtual void updateStrokeColorSpace(GfxState * state);
    virtual void updateFillColor(GfxState * state);
    virtual void updateStrokeColor(GfxState * state);


    /*
     * Rendering
     */

    virtual void clip(GfxState * state);
    virtual void eoClip(GfxState * state);
    virtual void clipToStrokePath(GfxState * state);
    
    virtual void drawString(GfxState * state, GooString * s);

    virtual void drawImage(GfxState * state, 
                           Object * ref, 
                           Stream * str, 
                           int width, 
                           int height, 
                           GfxImageColorMap * colorMap, 
                           GBool interpolate, 
                           int *maskColors, 
                           GBool inlineImg);

    virtual void drawSoftMaskedImage(GfxState *state, 
                                     Object *ref, 
                                     Stream *str,
                                     int width, 
                                     int height,
                                     GfxImageColorMap *colorMap,
                                     GBool interpolate,
                                     Stream *maskStr,
                                     int maskWidth, 
                                     int maskHeight,
                                     GfxImageColorMap *maskColorMap,
                                     GBool maskInterpolate);

    virtual void stroke(GfxState *state); 
    virtual void fill(GfxState *state);
    virtual void eoFill(GfxState *state);
    virtual GBool axialShadedFill(GfxState *state, GfxAxialShading *shading, double tMin, double tMax);

    TextPage& get_page () { return page; }

private:
    // add a new empty segment
    // the old one may be re-used if empty
    // if copy_state if false and an old segment is used, we do not copy the states
    // return if a new segment is created
    bool begin_segment(GfxState * state, bool copy_state = false);
    void end_segment();
    bool new_segment(GfxState * state, bool copy_state = false) { 
        end_segment(); 
        return begin_segment(state, copy_state);
    }

    void begin_word(GfxState * state);
    void end_word();
    void new_word(GfxState * state) { 
        end_word(); 
        begin_word(state); 
    }

    // copy GfxState to cur_segment.style
    void copy_state(GfxState * state);

    PDFDoc * cur_doc;
    int pageNum;

    TextWord * cur_word = nullptr;
    TextSegment * cur_segment = nullptr;

    TextPage page;
};


} //namespace pdf2htmlEX 


#endif // TEXTPAGEBUILDER_H__
