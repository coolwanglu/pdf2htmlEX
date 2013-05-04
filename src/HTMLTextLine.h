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
        // for optimzation
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

    void append_unicodes(const Unicode * u, int l);
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
private:
    void optimize(void);

    const Param & param;
    AllStateManager & all_manager;

    HTMLLineState line_state;
    double ascent, descent;
    double clip_x1, clip_y1;

    std::vector<State> states;
    std::vector<Offset> offsets;
    std::vector<Unicode> text;
};

} // namespace pdf2htmlEX
#endif //HTMLTEXTLINE_H__
