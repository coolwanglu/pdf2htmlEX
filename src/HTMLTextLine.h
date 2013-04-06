#ifndef TEXTLINEBUFFER_H__
#define TEXTLINEBUFFER_H__

#include <iostream>
#include <vector>

#include <CharTypes.h>

#include "Param.h"
#include "StateManager.h"
#include "HTMLState.h"

namespace pdf2htmlEX {

/*
 * Store a series of 
 *  - Text
 *  - Shift
 *  - State change
 * within a line
 */
class HTMLTextLine
{
public:
    HTMLTextLine (const Param & param, AllStateManater & all_manager) 
        : param(param), all_manager(all_manager) { }

    class State : public HTMLState {
    public:
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


    class Offset {
    public:
        Offset(size_t size_idx, double width)
            :start_idx(size_idx),width(width)
        { }
        size_t start_idx; // should put this Offset right before text[start_idx];
        double width;
    };

    void append_unicodes(const Unicode * u, int l);
    void append_offset(double width);
    void append_state(const HTMLState & html_state);
    void flush(std::ostream & out);

private:
    void optimize(void);

    const Param & param;
    AllStateManater & all_manager;

    double x, y;
    long long tm_id;

    std::vector<State> states;
    std::vector<Offset> offsets;
    std::vector<Unicode> text;

    // for flush
    std::vector<State*> stack;
};

} // namespace pdf2htmlEX
#endif //TEXTLINEBUFFER_H__
