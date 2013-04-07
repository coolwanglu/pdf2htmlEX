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

private:
    void optimize(void);

    const Param & param;
    AllStateManager & all_manager;
    HTMLTextLine * cur_line;
    std::vector<std::unique_ptr<HTMLTextLine>> text_lines;
};

} //namespace pdf2htmlEX 
#endif //HTMLTEXTPAGE_H__
