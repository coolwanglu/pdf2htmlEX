/*
 * Header file for HTMLTextLine
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */
#ifndef HTMLTEXTLINE_H__
#define HTMLTEXTLINE_H__

#include <ostream>
#include <vector>

#include <CharTypes.h>

#include "Param.h"
#include "StateManager.h"
#include "HTMLState.h"

namespace pdf2htmlEX {

/*
 * Store and optimize a line of text in HTML
 *
 * contains a series of 
 *  - Text
 *  - Shift
 *  - State change
 */
class HTMLTextLine
{
public:
    HTMLTextLine (const HTMLLineState & line_state, const Param & param, AllStateManager & all_manager);

    struct State : public HTMLTextState {
        // before output
        void begin(std::ostream & out, const State * prev_state);
        // after output
        void end(std::ostream & out) const;
        // calculate the hash code
        void hash(void);
        // calculate the difference between another State
        int diff(const State & s) const;

        enum {
            FONT_ID,
            FONT_SIZE_ID,
            FILL_COLOR_ID,
            STROKE_COLOR_ID,
            LETTER_SPACE_ID,
            WORD_SPACE_ID,
            HASH_ID_COUNT,

            VERTICAL_ALIGN_ID = HASH_ID_COUNT,
            ID_COUNT
        };

        static long long umask_by_id(int id);

        long long ids[ID_COUNT];

        size_t start_idx; // index of the first Text using this state
        // for optimization
        long long hash_value;
        long long hash_umask; // some states may not be actually used
        bool need_close;

        static const char * const css_class_names []; // class names for each id
    };

    struct Offset {
        Offset(size_t size_idx, double width)
            :start_idx(size_idx),width(width)
        { }
        size_t start_idx; // should put this Offset right before text[start_idx];
        double width;
    };

    /**
     * Append a drawn char (glyph)'s unicode. l > 1 mean this glyph correspond to
     * multiple code points.
     */
    void append_unicodes(const Unicode * u, int l, double width);
    /**
     * Append a special padding char with 0 width, in order to keep char index consistent.
     * The padding char is ignored during output.
     */
    void append_padding_char() { text.push_back(0); }
    void append_offset(double width);
    void append_state(const HTMLTextState & text_state);
    void dump_text(std::ostream & out);

    bool text_empty(void) const { return text.empty(); }
    void clear(void);

    void clip(const HTMLClipState &);

    /*
     * Optimize and calculate necessary values
     */
    void prepare(void);
    void optimize(std::vector<HTMLTextLine*> &);
private:
    void optimize_normal(std::vector<HTMLTextLine*> &);
    void optimize_aggressive(std::vector<HTMLTextLine*> &);

    /**
     * Dump chars' unicode to output stream.
     * begin/pos is the index in 'text'.
     */
    void dump_chars(std::ostream & out, int begin, int len);
    void dump_char(std::ostream & out, int pos);

    const Param & param;
    AllStateManager & all_manager;

    HTMLLineState line_state;
    double ascent, descent;
    double clip_x1, clip_y1;
    double width;

    std::vector<State> states;
    std::vector<Offset> offsets;

    /**
     * Drawn chars (glyph) in this line are stored in 'text'. For each element c in 'text':
     * - If c > 0, it is the unicode code point corresponds to the glyph;
     * - If c == 0, it is a padding char, and ignored during output (TODO some bad PDFs utilize 0?);
     * - If c < -1, this glyph corresponds to more than one unicode code points,
     *   which are stored in 'decomposed_text', and (-c-1) is the index in 'decomposed_text'.
     */
    std::vector<int> text;
    std::vector<std::vector<Unicode> > decomposed_text;
};

} // namespace pdf2htmlEX
#endif //HTMLTEXTLINE_H__
