/*
 * HTMLRenderer.h
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef HTMLRENDERER_H_
#define HTMLRENDERER_H_

#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <memory>

#include <OutputDev.h>
#include <GfxState.h>
#include <Stream.h>
#include <PDFDoc.h>
#include <goo/gtypes.h>
#include <Object.h>
#include <GfxFont.h>
#include <Annot.h>

// for form.cc
#include <Page.h>
#include <Form.h>

#include "pdf2htmlEX-config.h"

#include "Param.h"
#include "Preprocessor.h"
#include "StringFormatter.h"
#include "TmpFiles.h"
#include "Color.h"
#include "StateManager.h"
#include "HTMLTextPage.h"

#include "BackgroundRenderer/BackgroundRenderer.h"
#include "CoveredTextDetector.h"
#include "DrawingTracer.h"

#include "util/const.h"
#include "util/misc.h"


namespace pdf2htmlEX {

struct HTMLRenderer : OutputDev
{
    HTMLRenderer(const Param & param);
    virtual ~HTMLRenderer();

    void process(PDFDoc * doc);

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
    virtual GBool needNonText() { return (param.process_nontext) ? gTrue: gFalse; }

    // Does this device need to clip pages to the crop box even when the
    // box is the crop box?
    virtual GBool needClipToCropBox() { return gTrue; }

    virtual void setDefaultCTM(double *ctm);

    // Start a page.
    virtual void startPage(int pageNum, GfxState *state, XRef * xref);

    // End a page.
    virtual void endPage();

    /*
     * To optimize false alarms
     * We just mark as changed, and recheck if they have been changed when we are about to output a new string
     */

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

    virtual void drawImage(GfxState * state, Object * ref, Stream * str, int width, int height, GfxImageColorMap * colorMap, GBool interpolate, int *maskColors, GBool inlineImg);

    virtual void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
                       int width, int height,
                       GfxImageColorMap *colorMap,
                       GBool interpolate,
                       Stream *maskStr,
                       int maskWidth, int maskHeight,
                       GfxImageColorMap *maskColorMap,
                       GBool maskInterpolate);

    virtual void stroke(GfxState *state); 
    virtual void fill(GfxState *state);
    virtual void eoFill(GfxState *state);
    virtual GBool axialShadedFill(GfxState *state, GfxAxialShading *shading, double tMin, double tMax);

    virtual void processLink(AnnotLink * al);

    /*
     * Covered text handling.
     */
    // Is a char (actually a glyph) covered by non-char's. Index in drawing order in current page.
    // Does not fail on out-of-bound conditions, but return false.
    bool is_char_covered(int index);
    // Currently drawn char (glyph) count in current page.
    int get_char_count() { return (int)covered_text_detector.get_chars_covered().size(); }

protected:
    ////////////////////////////////////////////////////
    // misc
    ////////////////////////////////////////////////////
    void pre_process(PDFDoc * doc);
    void post_process(void);

    void process_outline(void);
    void process_outline_items(GooList * items);

    void process_form(std::ofstream & out);
    
    void set_stream_flags (std::ostream & out);

    void dump_css(void);

    // convert a LinkAction to a string that our Javascript code can understand
    std::string get_linkaction_str(LinkAction *, std::string & detail);

    ////////////////////////////////////////////////////
    /*
     * manage fonts
     *
     * In PDF: (install_*)
     * embedded font: fonts embedded in PDF
     * external font: fonts that have only names provided in PDF, the viewer should find a local font to match with
     *
     * In HTML: (export_*)
     * remote font: to be retrieved from the web server
     * remote default font: fallback styles for invalid fonts
     * local font: to be substituted with a local (client side) font
     */
    ////////////////////////////////////////////////////
    std::string dump_embedded_font(GfxFont * font, FontInfo & info);
    std::string dump_type3_font(GfxFont * font, FontInfo & info);
    void embed_font(const std::string & filepath, GfxFont * font, FontInfo & info, bool get_metric_only = false);
    const FontInfo * install_font(GfxFont * font);
    void install_embedded_font(GfxFont * font, FontInfo & info);
    void install_external_font (GfxFont * font, FontInfo & info);
    void export_remote_font(const FontInfo & info, const std::string & suffix, GfxFont * font);
    void export_remote_default_font(long long fn_id);
    void export_local_font(const FontInfo & info, GfxFont * font, const std::string & original_font_name, const std::string & cssfont);

    // depending on --embed***, to embed the content or add a link to it
    // "type": specify the file type, usually it's the suffix, in which case this parameter could be ""
    // "copy": indicates whether to copy the file into dest_dir, if not embedded
    void embed_file(std::ostream & out, const std::string & path, const std::string & type, bool copy);

    ////////////////////////////////////////////////////
    // state tracking 
    ////////////////////////////////////////////////////
    // reset all states
    void reset_state();
    // reset all ***_changed flags
    void reset_state_change();
    // check updated states, and determine new_line_status
    // make sure this function can be called several times consecutively without problem
    void check_state_change(GfxState * state);
    // prepare the line context, (close old tags, open new tags)
    // make sure the current HTML style consistent with PDF
    void prepare_text_line(GfxState * state);

    ////////////////////////////////////////////////////
    // PDF stuffs
    ////////////////////////////////////////////////////
    
    XRef * xref;
    PDFDoc * cur_doc;
    Catalog * cur_catalog;
    int pageNum;

    double default_ctm[6];

    /*
     * The content of each page is first scaled with factor1 (>=1), then scale back with factor2(<=1)
     *
     * factor1 is use to multiplied with all metrics (height/width/font-size...), in order to improve accuracy
     * factor2 is applied with css transform, and is exposed to Javascript
     *
     * factor1 & factor 2 are determined according to zoom and font-size-multiplier
     *
     */
    double text_zoom_factor (void) const { return text_scale_factor1 * text_scale_factor2; }
    double text_scale_factor1;
    double text_scale_factor2;

    // 1px on screen should be printed as print_scale()pt
    double print_scale (void) const { return 96.0 / DEFAULT_DPI / text_zoom_factor(); }


    const Param & param;

    ////////////////////////////////////////////////////
    // PDF states
    ////////////////////////////////////////////////////
    // track the original (unscaled) values to determine scaling and merge lines
    // current position
    double cur_tx, cur_ty; // real text position, in text coords
    double cur_font_size;
    // this is CTM * TextMAT in PDF
    // as we'll calculate the position of the origin separately
    double cur_text_tm[6]; // unscaled

    bool all_changed;
    bool ctm_changed;
    bool rise_changed;
    bool font_changed;
    bool text_pos_changed; 
    bool text_mat_changed;
    bool fill_color_changed;
    bool hori_scale_changed;
    bool word_space_changed;
    bool letter_space_changed;
    bool stroke_color_changed;
    bool clip_changed;

    ////////////////////////////////////////////////////
    // HTML states
    ////////////////////////////////////////////////////

    // optimize for web
    // we try to render the final font size directly
    // to reduce the effect of ctm as much as possible
    
    // the actual tm used is `real tm in PDF` scaled by 1/draw_text_scale, 
    // so everything rendered should be multiplied by draw_text_scale
    double draw_text_scale; 

    // the position of next char, in text coords
    // this is actual position (in HTML), which might be different from cur_tx/ty (in PDF)
    // also keep in mind that they are not the final position, as they will be transform by CTM (also true for cur_tx/ty)
    double draw_tx, draw_ty; 


    ////////////////////////////////////////////////////
    // styles & resources
    ////////////////////////////////////////////////////
    // managers store values actually used in HTML (i.e. scaled)
    std::unordered_map<long long, FontInfo> font_info_map;
    AllStateManager all_manager;
    HTMLTextState cur_text_state;
    HTMLLineState cur_line_state;
    HTMLClipState cur_clip_state;

    HTMLTextPage html_text_page;

    enum NewLineState
    {
        NLS_NONE,
        NLS_NEWSTATE, 
        NLS_NEWLINE,
        NLS_NEWCLIP
    } new_line_state;
    
    // for font reencoding
    std::vector<int32_t> cur_mapping; 
    std::vector<char*> cur_mapping2;
    std::vector<int> width_list; // width of each char

    Preprocessor preprocessor;

    // manage temporary files
    TmpFiles tmp_files;

    // for string formatting
    StringFormatter str_fmt;

    // render background image
    friend class SplashBackgroundRenderer; // ugly!
    friend class CairoBackgroundRenderer; // ugly!

    std::unique_ptr<BackgroundRenderer> bg_renderer, fallback_bg_renderer;

    struct {
        std::ofstream fs;
        std::string path;
    } f_outline, f_pages, f_css;
    std::ofstream * f_curpage;
    std::string cur_page_filename;

    static const std::string MANIFEST_FILENAME;

    CoveredTextDetector covered_text_detector;
    DrawingTracer tracer;
};

} //namespace pdf2htmlEX

#endif /* HTMLRENDERER_H_ */
