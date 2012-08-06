/*
 * HTMLRenderer.h
 *
 *  Created on: Mar 15, 2011
 *      Author: tian
 */

#ifndef HTMLRENDERER_H_
#define HTMLRENDERER_H_

#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <map>
#include <vector>

#include <OutputDev.h>
#include <GfxState.h>
#include <CharTypes.h>
#include <Stream.h>
#include <Array.h>
#include <Dict.h>
#include <XRef.h>
#include <Catalog.h>
#include <Page.h>
#include <PDFDoc.h>
#include <goo/gtypes.h>
#include <Object.h>
#include <GfxFont.h>

#include "Param.h"

using namespace std;

static const double EPS = 1e-6;
inline bool _equal(double x, double y) { return std::abs(x-y) < EPS; }
inline bool _is_positive(double x) { return x > EPS; }
inline bool _tm_equal(const double * tm1, const double * tm2, int size = 6)
{
    for(int i = 0; i < size; ++i)
        if(!_equal(tm1[i], tm2[i]))
            return false;
    return true;
}

class TextString
{
    public:
        TextString(GfxState *state);
        ~TextString();

        // Add a character to the string.
        void addChar(GfxState *state, double x, double y,
                double dx, double dy,
                Unicode u);
        double getX() const {return x;}
        double getY() const {return y;}
        double getWidth() const {return width;}
        double getHeight() const {return height;}

        GfxState * getState() const { return state; }

        const std::vector<Unicode> & getUnicodes() const { return unicodes; }
        size_t getSize() const { return unicodes.size(); }


    private:
        std::vector<Unicode> unicodes;
        double x, y;
        double width, height;

        // TODO: 
        // remove this, track state change in the converter
        GfxState * state;
};

class HTMLRenderer : public OutputDev
{
    public:
        HTMLRenderer(const Param * param);
        virtual ~HTMLRenderer();

        void process(PDFDoc * doc);

        //---- get info about output device

        // Does this device use upside-down coordinates?
        // (Upside-down means (0,0) is the top left corner of the page.)
        virtual GBool upsideDown() { return gFalse; }

        // Does this device use drawChar() or drawString()?
        virtual GBool useDrawChar() { return gTrue; }

        // Does this device use beginType3Char/endType3Char?  Otherwise,
        // text in Type 3 fonts will be drawn with drawChar/drawString.
        virtual GBool interpretType3Chars() { return gFalse; }

        // Does this device need non-text content?
        virtual GBool needNonText() { return gFalse; }

        //----- initialization and control
        virtual GBool checkPageSlice(Page *page, double hDPI, double vDPI,
                int rotate, GBool useMediaBox, GBool crop,
                int sliceX, int sliceY, int sliceW, int sliceH,
                GBool printing, Catalog * catalogA,
                GBool (* abortCheckCbk)(void *data) = NULL,
                void * abortCheckCbkData = NULL)
        {
            docPage = page;
            catalog = catalogA;
            return gTrue;
        }


        // Start a page.
        virtual void startPage(int pageNum, GfxState *state);

        // End a page.
        virtual void endPage();

        //----- update state
        virtual void updateAll(GfxState * state);
        virtual void updateFont(GfxState * state);
        virtual void updateTextMat(GfxState * state);
        virtual void updateCTM(GfxState * state, double m11, double m12, double m21, double m22, double m31, double m32);
        virtual void updateTextPos(GfxState * state);
        virtual void saveTextPos(GfxState * state);
        virtual void restoreTextPos(GfxState * state);

        //----- text drawing
        virtual void beginString(GfxState *state, GooString *s);
        virtual void endString(GfxState *state);
        virtual void drawChar(GfxState *state, double x, double y,
                double dx, double dy,
                double originX, double originY,
                CharCode code, int nBytes, Unicode *u, int uLen);

        virtual void drawString(GfxState * state, GooString * s);

    private:
        bool at_same_line(const TextString * ts1, const TextString * ts2) const;

        void close_cur_line();
        void outputTextString(TextString * str);

        // return the mapped font name
        long long install_font(GfxFont * font);

        static void output_to_file(void * outf, const char * data, int len);
        void install_embedded_type1_font (Ref * id, GfxFont * font, long long fn_id);
        void install_embedded_type1c_font (GfxFont * font, long long fn_id);
        void install_embedded_opentypet1c_font (GfxFont * font, long long fn_id);
        void install_embedded_truetype_font (GfxFont * font, long long fn_id);
        void install_base_font(GfxFont * font, GfxFontLoc * font_loc, long long fn_id);

        long long install_font_size(double font_size);
        long long install_whitespace(double ws_width, double & actual_width);
        long long install_transform_matrix(const double * tm);

        /*
         * remote font: to be retrieved from the web server
         * local font: to be substituted with a local (client side) font
         */
        void export_remote_font(long long fn_id, const string & suffix, GfxFont * font);
        void export_remote_default_font(long long fn_id);
        void export_local_font(long long fn_id, GfxFont * font, GfxFontLoc * font_loc, const string & original_font_name, const string & cssfont);
        std::string general_font_family(GfxFont * font);

        void export_font_size(long long fs_id, double font_size);
        void export_whitespace(long long ws_id, double ws_width);
        void export_transform_matrix(long long tm_id, const double * tm);

        Catalog *catalog;
        Page *docPage;

        // page info
        int pageNum ;
        double pageWidth ;
        double pageHeight ;


        // state maintained when processing pdf

        void check_state_change(GfxState * state);

        // current position
        double cur_line_y;
        bool pos_changed;

        // the string being processed
        TextString * cur_string;
        // the last word of current line
        // if it's not nullptr, there's an open <div> waiting for new strings in the same line
        TextString * cur_line;
        // (actual x) - (supposed x)
        double cur_line_x_offset;


        long long cur_fn_id;
        double cur_font_size;

        long long cur_fs_id; 
        bool font_changed;

        long long cur_tm_id;
        bool ctm_changed;
        bool text_mat_changed;


        // this is the modified fontsize & ctm for optimzation
        double draw_ctm[6];
        double draw_font_size;

        ofstream html_fout, allcss_fout;

        class FontInfo{
            public:
                long long fn_id;
        };
        unordered_map<long long, FontInfo> font_name_map;
        map<double, long long> font_size_map;
        map<double, long long> whitespace_map;
        XRef * xref;

        // transform matrix
        class TM{
            public:
                TM() {}
                TM(const double * m) {memcpy(_, m, sizeof(_));}
                bool operator < (const TM & m) const {
                    for(int i = 0; i < 6; ++i)
                    {
                        if(_[i] < m._[i] - EPS)
                            return true;
                        if(_[i] > m._[i] + EPS)
                            return false;
                    }
                    return false;
                }
                bool operator == (const TM & m) const {
                    return _tm_equal(_, m._);
                }
                double _[6];
        };

        map<TM, long long> transform_matrix_map;

        const Param * param;
};

#endif /* HTMLRENDERER_H_ */
