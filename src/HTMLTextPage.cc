/*
 * HTMLTextPage.cc
 *
 * Generate and optimized HTML for one Page
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#include "HTMLTextPage.h"

namespace pdf2htmlEX {

using std::ostream;
using std::unique_ptr;

HTMLTextPage::HTMLTextPage(const Param & param, AllStateManager & all_manager)
    : param(param)
    , all_manager(all_manager)
    , cur_line(nullptr)
{ } 

void HTMLTextPage::dump_text(ostream & out)
{
    for(auto iter = text_lines.begin(); iter != text_lines.end(); ++iter)
        (*iter)->prepare();
    if(param.optimize_text)
        optimize();
    for(auto iter = text_lines.begin(); iter != text_lines.end(); ++iter)
        (*iter)->dump_text(out);
}

void HTMLTextPage::dump_css(ostream & out)
{
    //TODO
}

void HTMLTextPage::clear(void)
{
    text_lines.clear();
    cur_line = nullptr;
}

void HTMLTextPage::open_new_line(const HTMLLineState & line_state)
{
    if((!text_lines.empty()) && (text_lines.back()->text_empty()))
    {
        text_lines.pop_back();
    }
    text_lines.emplace_back(new HTMLTextLine(line_state, param, all_manager));
    cur_line = text_lines.back().get();
}

void HTMLTextPage::optimize(void)
{
    //TODO
    //group lines with same x-axis
    //collect common states 
}

} // namespace pdf2htmlEX
