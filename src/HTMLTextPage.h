/*
 * Header file for HTMLTextPage
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef HTMLTEXTPAGE_H__
#define HTMLTEXTPAGE_H__

#include <vector>
#include <ostream>

#include "Param.h"
#include "StateManager.h"
#include "HTMLTextLine.h"
#include "HTMLState.h"

namespace pdf2htmlEX {

/*
 * Store and optimize a page of text in HTML
 *
 * contains a series of HTMLTextLine
 */
class HTMLTextPage
{
public:
    HTMLTextPage (const Param & param, AllStateManager & all_manager);
    ~HTMLTextPage();

    HTMLTextLine * get_cur_line(void) const { return cur_line; }

    void dump_text(std::ostream & out);
    void dump_css(std::ostream & out);
    void clear(void);

    void open_new_line(const HTMLLineState & line_state);
    
    /* for clipping */
    void set_page_size(double width, double height);
    void clip(const HTMLClipState & clip_state);

    double get_width() { return page_width; }
    double get_height() { return page_height; }

private:
    void optimize(void);

    const Param & param;
    AllStateManager & all_manager;
    HTMLTextLine * cur_line;
    double page_width, page_height;

    std::vector<HTMLTextLine*> text_lines;

    struct Clip {
        HTMLClipState clip_state;
        size_t start_idx;
        Clip(const HTMLClipState & clip_state, size_t start_idx)
            :clip_state(clip_state),start_idx(start_idx)
        { }
    };
    std::vector<Clip> clips;
};

} //namespace pdf2htmlEX 
#endif //HTMLTEXTPAGE_H__
