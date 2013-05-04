/*
 * Header file for HTMLTextPage
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef HTMLTEXTPAGE_H__
#define HTMLTEXTPAGE_H__

#include <vector>
#include <memory>
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

    HTMLTextLine * get_cur_line(void) const { return cur_line; }

    void dump_text(std::ostream & out);
    void dump_css(std::ostream & out);
    void clear(void);

    void open_new_line(const HTMLLineState & line_state);
    
    /* for clipping */
    void set_page_size(double width, double height);
    void clip(double x1, double y1, double x2, double y2);

private:
    void optimize(void);

    const Param & param;
    AllStateManager & all_manager;
    HTMLTextLine * cur_line;
    double page_width, page_height;

    std::vector<std::unique_ptr<HTMLTextLine>> text_lines;

    struct ClipBox {
        ClipBox(double x1, double y1, double x2, double y2, size_t start_idx)
            :x1(x1),y1(y1),x2(x2),y2(y2),start_idx(start_idx)
        { }
        double x1, y1, x2, y2;
        size_t start_idx;
    };
    std::vector<ClipBox> clip_boxes;
};

} //namespace pdf2htmlEX 
#endif //HTMLTEXTPAGE_H__
