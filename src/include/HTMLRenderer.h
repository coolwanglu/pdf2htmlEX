/*
 * HTMLRenderer.h
 *
 * by WangLu
 */

#ifndef HTMLRENDERER_H_
#define HTMLRENDERER_H_

#include <unordered_map>
#include <map>
#include <vector>
#include <set>
#include <sstream>
#include <cstdint>
#include <fstream>

#include <OutputDev.h>
#include <GfxState.h>
#include <Stream.h>
#include <XRef.h>
#include <Catalog.h>
#include <Page.h>
#include <PDFDoc.h>
#include <goo/gtypes.h>
#include <Object.h>
#include <GfxFont.h>

#include "Param.h"
#include "util.h"
#include "FontPreprocessor.h"

/*
 * Naming Convention
 *
 * CSS classes
 *
 * b - page Box
 * p - Page
 * l - Line
 * _ - white space
 * i - Image
 *
 * Reusable CSS classes
 *
 * t<hex> - Transform matrix
 * f<hex> - Font (also for font names)
 * s<hex> - font Size
 * l<hex> - Letter spacing
 * w<hex> - Word spacing
 * c<hex> - Color
 * _<hex> - white space
 * r<hex> - Rise
 * h<hex> - Height
 *
 */

namespace pdf2htmlEX {

class HTMLRenderer : public OutputDev
{
    public:
        HTMLRenderer(const Param * param);
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

        // Does this device use beginType3Char/endType3Char?  Otherwise,
        // text in Type 3 fonts will be drawn with drawChar/drawString.
        virtual GBool interpretType3Chars() { return gFalse; }

        // Does this device need non-text content?
        virtual GBool needNonText() { return gFalse; }

        virtual void pre_process();
        virtual void post_process();

        // Start a page.
        virtual void startPage(int pageNum, GfxState *state);

        // End a page.
        virtual void endPage();

        /*
         * To optmize false alarms
         * We just mark as changed, and recheck if they have been changed when we are about to output a new string
         */
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

        virtual void updateFillColor(GfxState * state);


        /*
         * Rendering
         */
        
        virtual void drawString(GfxState * state, GooString * s);

        virtual void drawImage(GfxState * state, Object * ref, Stream * str, int width, int height, GfxImageColorMap * colorMap, GBool interpolate, int *maskColors, GBool inlineImg);

    protected:
        ////////////////////////////////////////////////////
        // misc
        ////////////////////////////////////////////////////
        
        // set flags 
        void fix_stream (std::ostream & out);

        void add_tmp_file (const std::string & fn);
        void clean_tmp_files ();
        std::string dump_embedded_font (GfxFont * font, long long fn_id);
        void embed_font(const std::string & filepath, GfxFont * font, FontInfo & info, bool get_metric_only = false);

        ////////////////////////////////////////////////////
        // manage styles
        ////////////////////////////////////////////////////
        const FontInfo * install_font(GfxFont * font);
        void install_embedded_font(GfxFont * font, FontInfo & info);
        void install_base_font(GfxFont * font, GfxFontLoc * font_loc, FontInfo & info);
        void install_external_font (GfxFont * font, FontInfo & info);

        long long install_font_size(double font_size);
        long long install_transform_matrix(const double * tm);
        long long install_letter_space(double letter_space);
        long long install_word_space(double word_space);
        long long install_color(const GfxRGB * rgb);
        long long install_whitespace(double ws_width, double & actual_width);
        long long install_rise(double rise);
        long long install_height(double height);

        ////////////////////////////////////////////////////
        // export css styles
        ////////////////////////////////////////////////////
        /*
         * remote font: to be retrieved from the web server
         * local font: to be substituted with a local (client side) font
         */
        void export_remote_font(const FontInfo & info, const std::string & suffix, const std::string & fontfileformat, GfxFont * font);
        void export_remote_default_font(long long fn_id);
        void export_local_font(const FontInfo & info, GfxFont * font, const std::string & original_font_name, const std::string & cssfont);

        void export_font_size(long long fs_id, double font_size);
        void export_transform_matrix(long long tm_id, const double * tm);
        void export_letter_space(long long ls_id, double letter_space);
        void export_word_space(long long ws_id, double word_space);
        void export_color(long long color_id, const GfxRGB * rgb);
        void export_whitespace(long long ws_id, double ws_width);
        void export_rise(long long rise_id, double rise);
        void export_height(long long height_id, double height);

        // depending on single-html, to embed the content or add a link to it
        // "type": specify the file type, usually it's the suffix, in which case this parameter could be ""
        // "copy": indicates whether to copy the file into dest_dir, if not embedded
        void embed_file(std::ostream & out, const std::string & path, const std::string & type, bool copy);

        ////////////////////////////////////////////////////
        // state tracking 
        ////////////////////////////////////////////////////
        // check updated states, and determine new_line_stauts
        // make sure this function can be called several times consecutively without problem
        void check_state_change(GfxState * state);
        // reset all ***_changed flags
        void reset_state_change();
        // prepare the line context, (close old tags, open new tags)
        // make sure the current HTML style consistent with PDF
        void prepare_line(GfxState * state);
        void close_line();


        ////////////////////////////////////////////////////
        // PDF stuffs
        ////////////////////////////////////////////////////
        
        XRef * xref;

        // page info
        int pageNum;
        double pageWidth ;
        double pageHeight ;


        ////////////////////////////////////////////////////
        // states
        ////////////////////////////////////////////////////
        bool line_opened;
        enum NewLineState
        {
            NLS_NONE, // stay with the same style
            NLS_SPAN, // open a new <span> if possible, otherwise a new <div>
            NLS_DIV   // has to open a new <div>
        } new_line_state;
        
        // The order is according to the appearance in check_state_change
        // any state changed
        bool all_changed;
        // current position
        double cur_tx, cur_ty; // real text position, in text coords
        bool text_pos_changed; 

        // font & size
        const FontInfo * cur_font_info;
        double cur_font_size;
        long long cur_fs_id; 
        bool font_changed;

        // transform matrix
        long long cur_tm_id;
        bool ctm_changed;
        bool text_mat_changed;
        // horizontal scaling
        bool hori_scale_changed;
        // this is CTM * TextMAT in PDF, not only CTM
        // [4] and [5] are ignored,
        // as we'll calculate the position of the origin separately
        // TODO: changed this for images
        double cur_ctm[6]; // unscaled

        // letter spacing 
        long long cur_ls_id;
        double cur_letter_space;
        bool letter_space_changed;

        // word spacing
        long long cur_ws_id;
        double cur_word_space;
        bool word_space_changed;

        // color
        long long cur_color_id;
        GfxRGB cur_color;
        bool color_changed;

        // rise
        long long cur_rise_id;
        double cur_rise;
        bool rise_changed;

        // optimize for web
        // we try to render the final font size directly
        // to reduce the effect of ctm as much as possible
        
        // draw_ctm is cur_ctm scaled by 1/draw_scale, 
        // so everything redenered should be multiplied by draw_scale
        double draw_ctm[6];
        double draw_font_size;
        double draw_scale; 

        // the position of next char, in text coords
        // this is actual position (in HTML), which might be different from cur_tx/ty (in PDF)
        // also keep in mind that they are not the final position, as they will be transform by CTM (also true for cur_tx/ty)
        double draw_tx, draw_ty; 

        // some metrics have to be determined after all elements in the lines have been seen
        class LineBuffer {
        public:
            LineBuffer (HTMLRenderer * renderer) : renderer(renderer) { }

            class State {
            public:
                void begin(std::ostream & out, const State * prev_state);
                void end(std::ostream & out) const;
                void hash(void);
                int diff(const State & s) const;

                enum {
                    FONT_ID,
                    FONT_SIZE_ID,
                    COLOR_ID,
                    LETTER_SPACE_ID,
                    WORD_SPACE_ID,
                    RISE_ID,

                    ID_COUNT
                };

                long long ids[ID_COUNT];

                double ascent;
                double descent;
                double draw_font_size;

                size_t start_idx; // index of the first Text using this state
                // for optimzation
                long long hash_value;
                bool need_close;

                static const char * format_str; // class names for each id
            };


            class Offset {
            public:
                size_t start_idx; // should put this idx before text[start_idx];
                double width;
            };

            void reset(GfxState * state);
            void append_unicodes(const Unicode * u, int l);
            void append_offset(double width);
            void append_state(void);
            void flush(void);

        private:
            // retrieve state from renderer
            void set_state(State & state);

            HTMLRenderer * renderer;

            double x, y;
            long long tm_id;

            std::vector<State> states;
            std::vector<Offset> offsets;
            std::vector<Unicode> text;

            // for flush
            std::vector<State*> stack;

        } line_buf;
        friend class LineBuffer;

        // for font reencoding
        int32_t * cur_mapping;
        char ** cur_mapping2;
        FontPreprocessor font_preprocessor;

        // for string formatting
        string_formatter str_fmt;

        ////////////////////////////////////////////////////
        // styles & resources
        ////////////////////////////////////////////////////

        std::unordered_map<long long, FontInfo> font_name_map;
        std::map<double, long long> font_size_map;
        std::map<TM, long long> transform_matrix_map;
        std::map<double, long long> letter_space_map;
        std::map<double, long long> word_space_map;
        std::unordered_map<GfxRGB, long long, GfxRGB_hash, GfxRGB_equal> color_map; 
        std::map<double, long long> whitespace_map;
        std::map<double, long long> rise_map;
        std::map<double, long long> height_map;

        int image_count;

        const Param * param;
        std::ofstream html_fout, css_fout;
        std::string html_path, css_path;
        std::set<std::string> tmp_files;

        static const std::string MANIFEST_FILENAME;
};

} //namespace pdf2htmlEX

#endif /* HTMLRENDERER_H_ */
