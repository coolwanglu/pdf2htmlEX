#ifndef TEXTLINEBUFFER_H__
#define TEXTLINEBUFFER_H__

#include <iostream>
#include <vector>

namespace pdf2htmlEX {

/*
 * Store a series of 
 *  - Text
 *  - Shift
 *  - State change
 * within a line
 */
class HTMLRenderer;
class HTMLRenderer::TextLineBuffer
{
public:
    TextLineBuffer (HTMLRenderer * renderer) : renderer(renderer) { }

    class State {
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
            size_t start_idx; // should put this Offset right before text[start_idx];
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
};

} // namespace pdf2htmlEX
#endif //TEXTLINEBUFFER_H__
