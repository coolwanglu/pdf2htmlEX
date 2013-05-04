/*
 * HTMLTextPage.cc
 *
 * Generate and optimized HTML for one Page
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#include "HTMLTextPage.h"
#include "util/css_const.h"

namespace pdf2htmlEX {

using std::ostream;
using std::unique_ptr;

HTMLTextPage::HTMLTextPage(const Param & param, AllStateManager & all_manager)
    : param(param)
    , all_manager(all_manager)
    , cur_line(nullptr)
    , page_width(0)
    , page_height(0)
{ } 

void HTMLTextPage::dump_text(ostream & out)
{
    for(auto iter = text_lines.begin(); iter != text_lines.end(); ++iter)
        (*iter)->prepare();
    if(param.optimize_text)
        optimize();

    //push a dummy entry for convenience
    clip_boxes.emplace_back(0, 0, page_width, page_height, text_lines.size());

    ClipBox cur_cb(0, 0, page_width, page_height, 0);
    bool has_clip = false;

    auto text_line_iter = text_lines.begin();
    for(auto clip_iter = clip_boxes.begin(); clip_iter != clip_boxes.end(); ++clip_iter)
    {
        if(has_clip)
        {
            out << "<div class=\"" << CSS::CLIP_CN
                << " " << CSS::LEFT_CN   << all_manager.left.install(cur_cb.x1)
                << " " << CSS::BOTTOM_CN << all_manager.bottom.install(cur_cb.y1)
                << " " << CSS::WIDTH_CN  << all_manager.width.install(cur_cb.x2 - cur_cb.x1)
                << " " << CSS::HEIGHT_CN << all_manager.height.install(cur_cb.y2 - cur_cb.y1)
                << "\">";
        }

        auto next_text_line_iter = text_lines.begin() + clip_iter->start_idx;
        while(text_line_iter != next_text_line_iter)
        {
            (*text_line_iter)->clip(cur_cb.x1, cur_cb.y1, cur_cb.x2, cur_cb.y2);
            (*text_line_iter)->dump_text(out);
            ++text_line_iter;
        }
        if(has_clip)
        {
            out << "</div>";
        }

        cur_cb = *clip_iter;
        has_clip = !(equal(0, cur_cb.x1) && equal(0, cur_cb.y1) 
                && equal(page_width, cur_cb.x2) && equal(page_height, cur_cb.y2));
    }
}

void HTMLTextPage::dump_css(ostream & out)
{
    //TODO
}

void HTMLTextPage::clear(void)
{
    text_lines.clear();
    clip_boxes.clear();
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

void HTMLTextPage::set_page_size(double width, double height)
{
    page_width = width;
    page_height = height;
}

void HTMLTextPage::clip(double x1, double y1, double x2, double y2)
{
    if(!clip_boxes.empty())
    {
        auto & cb = clip_boxes.back();
        if(cb.start_idx == text_lines.size())
        {
            /*
             * Previous ClipBox is not used
             */
            cb.x1 = x1;
            cb.y1 = y1;
            cb.x2 = x2;
            cb.y2 = y2;
            return;
        }
        if(equal(cb.x1, x1) && equal(cb.y1, y1)
                && equal(cb.x2, x2) && equal(cb.y2, y2))
        {
            /*
             * same as previous ClipBox
             */
            return;
        }
    }
    clip_boxes.emplace_back(x1, y1, x2, y2, text_lines.size());
}

void HTMLTextPage::optimize(void)
{
    //TODO
    //group lines with same x-axis
    //collect common states 
}

} // namespace pdf2htmlEX
