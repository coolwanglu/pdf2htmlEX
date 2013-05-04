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
    clips.emplace_back(HTMLClipState{0, 0, page_width, page_height}, text_lines.size());

    Clip cur_clip(HTMLClipState{0, 0, page_width, page_height}, 0);
    bool has_clip = false;

    auto text_line_iter = text_lines.begin();
    for(auto clip_iter = clips.begin(); clip_iter != clips.end(); ++clip_iter)
    {
        auto next_text_line_iter = text_lines.begin() + clip_iter->start_idx;
        if(text_line_iter != next_text_line_iter)
        {
            const auto & cs = cur_clip.clip_state;
            if(has_clip)
            {
                out << "<div class=\"" << CSS::CLIP_CN
                    << " " << CSS::LEFT_CN   << all_manager.left.install(cs.xmin)
                    << " " << CSS::BOTTOM_CN << all_manager.bottom.install(cs.ymin)
                    << " " << CSS::WIDTH_CN  << all_manager.width.install(cs.xmax - cs.xmin)
                    << " " << CSS::HEIGHT_CN << all_manager.height.install(cs.ymax - cs.ymin)
                    << "\">";
            }

            while(text_line_iter != next_text_line_iter)
            {
                if(has_clip)
                {
                    (*text_line_iter)->clip(cs);
                }
                (*text_line_iter)->dump_text(out);
                ++text_line_iter;
            }
            if(has_clip)
            {
                out << "</div>";
            }
        }

        {
            cur_clip = *clip_iter;
            const auto & cs = cur_clip.clip_state;
            has_clip = !(equal(0, cs.xmin) && equal(0, cs.ymin)
                    && equal(page_width, cs.xmax) && equal(page_height, cs.ymax));
        }
    }
}

void HTMLTextPage::dump_css(ostream & out)
{
    //TODO
}

void HTMLTextPage::clear(void)
{
    text_lines.clear();
    clips.clear();
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

void HTMLTextPage::clip(const HTMLClipState & clip_state)
{
    if(!clips.empty())
    {
        auto & clip = clips.back();
        if(clip.start_idx == text_lines.size())
        {
            /*
             * Previous ClipBox is not used
             */
            clip.clip_state = clip_state;
            return;
        }
    }
    clips.emplace_back(clip_state, text_lines.size());
}

void HTMLTextPage::optimize(void)
{
    //TODO
    //group lines with same x-axis
    //collect common states 
}

} // namespace pdf2htmlEX
