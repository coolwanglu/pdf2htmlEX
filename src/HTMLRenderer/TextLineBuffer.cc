/*
 * TextLineBuffer.cc
 *
 * Generate and optimized HTML for one line
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <vector>

#include "HTMLRenderer.h"
#include "TextLineBuffer.h"
#include "util/namespace.h"
#include "util/unicode.h"
#include "util/math.h"

namespace pdf2htmlEX {

using std::min;
using std::max;
using std::vector;
using std::ostream;
using std::cerr;
using std::endl;

void HTMLRenderer::TextLineBuffer::reset(GfxState * state)
{
    state->transform(state->getCurX(), state->getCurY(), &x, &y);
    tm_id = renderer->cur_ttm_id;
}

void HTMLRenderer::TextLineBuffer::append_unicodes(const Unicode * u, int l)
{
    text.insert(text.end(), u, u+l);
}

void HTMLRenderer::TextLineBuffer::append_offset(double width)
{
    if((!offsets.empty()) && (offsets.back().start_idx == text.size()))
        offsets.back().width += width;
    else
        offsets.push_back(Offset({text.size(), width}));
}

void HTMLRenderer::TextLineBuffer::append_state(void)
{
    if(states.empty() || (states.back().start_idx != text.size()))
    {
        states.resize(states.size() + 1);
        states.back().start_idx = text.size();
    }

    set_state(states.back());
}

void HTMLRenderer::TextLineBuffer::flush(void)
{
    /*
     * Each Line is an independent absolute positioned block
     * so even we have a few states or offsets, we may omit them
     */
    if(text.empty()) return;

    if(states.empty() || (states[0].start_idx != 0))
    {
        cerr << "Warning: text without a style! Must be a bug in pdf2htmlEX" << endl;
        return;
    }

    for(auto iter = states.begin(); iter != states.end(); ++iter)
        iter->hash();

    states.resize(states.size() + 1);
    states.back().start_idx = text.size();

    offsets.push_back(Offset({text.size(), 0}));

    double max_ascent = 0;
    for(auto iter = states.begin(); iter != states.end(); ++iter)
    {
        const auto & s = *iter;
        max_ascent = max<double>(max_ascent, s.ascent * s.draw_font_size);
    }

    ostream & out = renderer->f_pages.fs;
    out << "<div style=\""
        << "bottom:" << round(y) << "px;"
        << "\""
        << " class=\"l"
        << " t" << tm_id 
        << " L" << renderer->install_left(x)
        << " h" << renderer->install_height(max_ascent)
        << "\">";

    auto cur_state_iter = states.begin();
    auto cur_offset_iter = offsets.begin();

    //accumulated horizontal offset;
    double dx = 0;

    stack.clear();
    stack.push_back(nullptr);

    // whenever a negative offset appears, we should not pop out that <span>
    // otherwise the effect of negative margin-left would disappear
    size_t last_text_pos_with_negative_offset = 0;

    size_t cur_text_idx = 0;
    while(cur_text_idx < text.size())
    {
        if(cur_text_idx >= cur_state_iter->start_idx)
        {
            // greedy
            int best_cost = State::ID_COUNT;
            
            // we have a nullptr at the beginning, so no need to check for rend
            for(auto iter = stack.rbegin(); *iter; ++iter)
            {
                int cost = cur_state_iter->diff(**iter);
                if(cost < best_cost)
                {
                    while(stack.back() != *iter)
                    {
                        stack.back()->end(out);
                        stack.pop_back();
                    }
                    best_cost = cost;

                    if(best_cost == 0)
                        break;
                }

                // cannot go further
                if((*iter)->start_idx <= last_text_pos_with_negative_offset)
                    break;
            }
            cur_state_iter->begin(out, stack.back());
            stack.push_back(&*cur_state_iter);

            ++ cur_state_iter;
        }

        if(cur_text_idx >= cur_offset_iter->start_idx)
        {
            double target = cur_offset_iter->width + dx;
            double w;

            auto wid = renderer->install_whitespace(target, w);

            if(w < 0)
                last_text_pos_with_negative_offset = cur_text_idx;

            auto * p = stack.back();
            double threshold = p->draw_font_size * (p->ascent - p->descent) * (renderer->param->space_threshold);

            out << "<span class=\"_ _" << wid << "\">" << (target > (threshold - EPS) ? " " : "") << "</span>";

            dx = target - w;

            ++ cur_offset_iter;
        }

        size_t next_text_idx = min<size_t>(cur_state_iter->start_idx, cur_offset_iter->start_idx);

        outputUnicodes(out, (&text.front()) + cur_text_idx, next_text_idx - cur_text_idx);
        cur_text_idx = next_text_idx;
    }

    // we have a nullptr in the bottom
    while(stack.back())
    {
        stack.back()->end(out);
        stack.pop_back();
    }

    out << "</div>";


    states.clear();
    offsets.clear();
    text.clear();

}

void HTMLRenderer::TextLineBuffer::set_state (State & state)
{
    state.ids[State::FONT_ID] = renderer->cur_font_info->id;
    state.ids[State::FONT_SIZE_ID] = renderer->font_size_manager.get_id();
    state.ids[State::FILL_COLOR_ID] = renderer->cur_fill_color_id;
    state.ids[State::STROKE_COLOR_ID] = renderer->cur_stroke_color_id;
    state.ids[State::LETTER_SPACE_ID] = renderer->letter_space_manager.get_id();
    state.ids[State::WORD_SPACE_ID] = renderer->word_space_manager.get_id();
    state.ids[State::RISE_ID] = renderer->rise_manager.get_id();

    const FontInfo * info = renderer->cur_font_info;
    state.ascent = info->ascent;
    state.descent = info->descent;
    state.draw_font_size = renderer->font_size_manager.get_value();
}

void HTMLRenderer::TextLineBuffer::State::begin (ostream & out, const State * prev_state)
{
    bool first = true;
    for(int i = 0; i < ID_COUNT; ++i)
    {
        if(prev_state && (prev_state->ids[i] == ids[i]))
            continue;

        if(first)
        { 
            out << "<span class=\"";
            first = false;
        }
        else
        {
            out << ' ';
        }

        // out should have hex set
        if (ids[i] == -1)
        {
            // transparent
            out << format_str[i] << "t";
        }
        else
        {
            out << format_str[i] << ids[i];
        }
    }

    if(first)
    {
        need_close = false;
    }
    else
    {
        out << "\">";
        need_close = true;
    }
}

void HTMLRenderer::TextLineBuffer::State::end(ostream & out) const
{
    if(need_close)
        out << "</span>";
}

void HTMLRenderer::TextLineBuffer::State::hash(void)
{
    hash_value = 0;
    for(int i = 0; i < ID_COUNT; ++i)
    {
        hash_value = (hash_value << 8) | (ids[i] & 0xff);
    }
}

int HTMLRenderer::TextLineBuffer::State::diff(const State & s) const
{
    /*
     * A quick check based on hash_value
     * it could be wrong when there are more then 256 classes, 
     * in which case the output may not be optimal, but still 'correct' in terms of HTML
     */
    if(hash_value == s.hash_value) return 0;

    int d = 0;
    for(int i = 0; i < ID_COUNT; ++i)
        if(ids[i] != s.ids[i])
            ++ d;
    return d;
}

const char * HTMLRenderer::TextLineBuffer::State::format_str = "fscClwr";
} //namespace pdf2htmlEX
