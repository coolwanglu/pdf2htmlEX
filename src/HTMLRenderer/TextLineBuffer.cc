/*
 * TextLineBuffer.cc
 *
 * Generate and optimized HTML for one line
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <vector>
#include <cmath>
#include <algorithm>

#include "HTMLRenderer.h"
#include "TextLineBuffer.h"
#include "util/namespace.h"
#include "util/unicode.h"
#include "util/math.h"
#include "util/css_const.h"
#include "util/encoding.h"

namespace pdf2htmlEX {

using std::min;
using std::max;
using std::vector;
using std::ostream;
using std::cerr;
using std::endl;
using std::find;
using std::abs;

void HTMLRenderer::TextLineBuffer::reset(GfxState * state)
{
    state->transform(state->getCurX(), state->getCurY(), &x, &y);
    tm_id = renderer->transform_matrix_manager.get_id();
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
        states.back().hash_umask = 0;
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

    optimize();

    double max_ascent = 0;
    for(auto iter = states.begin(); iter != states.end(); ++iter)
    {
        const auto & s = *iter;
        max_ascent = max<double>(max_ascent, s.font_info->ascent * s.draw_font_size);
    }

    // append a dummy state for convenience
    states.resize(states.size() + 1);
    states.back().start_idx = text.size();
    
    for(auto iter = states.begin(); iter != states.end(); ++iter)
        iter->hash();

    // append a dummy offset for convenience
    offsets.push_back(Offset({text.size(), 0}));

    ostream & out = renderer->f_pages.fs;
    renderer->height_manager.install(max_ascent);
    renderer->left_manager  .install(x);
    renderer->bottom_manager.install(y);

    out << "<div class=\"" << CSS::LINE_CN
        << " "             << CSS::TRANSFORM_MATRIX_CN << tm_id 
        << " "             << CSS::LEFT_CN             << renderer->left_manager  .get_id()
        << " "             << CSS::HEIGHT_CN           << renderer->height_manager.get_id()
        << " "             << CSS::BOTTOM_CN           << renderer->bottom_manager.get_id()
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

            if(equal(target, 0))
            {
                dx = 0;
            }
            else if(equal(target, stack.back()->single_space_offset()))
            {
                Unicode u = ' ';
                outputUnicodes(out, &u, 1);
                dx = 0;
            }
            else
            {
                auto & wm = renderer->whitespace_manager;
                wm.install(target);
                auto wid = wm.get_id();
                double w = wm.get_actual_value();

                if(w < 0)
                    last_text_pos_with_negative_offset = cur_text_idx;

                auto * p = stack.back();
                double threshold = p->draw_font_size * (p->font_info->ascent - p->font_info->descent) * (renderer->param->space_threshold);

                out << "<span class=\"" << CSS::WHITESPACE_CN
                    << ' ' << CSS::WHITESPACE_CN << wid << "\">" << (target > (threshold - EPS) ? " " : "") << "</span>";

                dx = target - w;
            }

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
    state.ids[State::FILL_COLOR_ID] = renderer->fill_color_manager.get_id();
    state.ids[State::STROKE_COLOR_ID] = renderer->stroke_color_manager.get_id();
    state.ids[State::LETTER_SPACE_ID] = renderer->letter_space_manager.get_id();
    state.ids[State::WORD_SPACE_ID] = renderer->word_space_manager.get_id();
    state.ids[State::RISE_ID] = renderer->rise_manager.get_id();

    state.font_info = renderer->cur_font_info;
    state.draw_font_size = renderer->font_size_manager.get_actual_value();
    state.letter_space = renderer->letter_space_manager.get_actual_value();
    state.word_space = renderer->word_space_manager.get_actual_value();
}

void HTMLRenderer::TextLineBuffer::optimize(void)
{
    // this function needs more work
    return;

    assert(!states.empty());

    // set proper hash_umask
    long long word_space_umask = ((long long)0xff) << (8*((int)State::WORD_SPACE_ID));
    for(auto iter = states.begin(); iter != states.end(); ++iter)
    {
        auto text_iter1 = text.begin() + (iter->start_idx);
        auto next_iter = iter;
        ++next_iter;
        auto text_iter2 =  (next_iter == states.end()) ? (text.end()) : (text.begin() + (next_iter->start_idx));
        if(find(text_iter1, text_iter2, ' ') == text_iter2)
        {
            // if there's no space, word_space does not matter;
            iter->hash_umask |= word_space_umask;
        }
    }

    // clean zero offsets
    {
        auto write_iter = offsets.begin();
        for(auto iter = offsets.begin(); iter != offsets.end(); ++iter)
        {
            if(!equal(iter->width, 0))
            {
                *write_iter = *iter;
                ++write_iter;
            }
        }
        offsets.erase(write_iter, offsets.end());
    }
    
    // In some PDF files all spaces are converted into positionig shifts
    // We may try to change them to ' ' and adjusted word_spaces
    // This can also be applied when param->space_as_offset is set

    // for now, we cosider only the no-space scenario
    if(offsets.size() > 0)
    {
        // Since GCC 4.4.6 is suported, I cannot use all_of + lambda here
        bool all_ws_umask = true;
        for(auto iter = states.begin(); iter != states.end(); ++iter)
        {
            if(!(iter->hash_umask & word_space_umask))
            {
                all_ws_umask = false;
                break;
            }
        }
        if(all_ws_umask)
        {
            double avg_width = 0;
            int posive_offset_count = 0;
            for(auto iter = offsets.begin(); iter != offsets.end(); ++iter)
            {
                if(is_positive(iter->width))
                {
                    ++posive_offset_count;
                    avg_width += iter->width;
                }
            }
            avg_width /= posive_offset_count;

            // now check if the width of offsets are close enough
            // TODO: it might make more sense if the threshold is proportion to the font size
            bool ok = true;
            double accum_off = 0;
            double orig_accum_off = 0;
            for(auto iter = offsets.begin(); iter != offsets.end(); ++iter)
            {
                orig_accum_off += iter->width;
                accum_off += avg_width;
                if(is_positive(iter->width) && abs(orig_accum_off - accum_off) >= renderer->param->h_eps)
                {
                    ok = false;
                    break;
                }
            }
            if(ok)
            {
                // ok, make all offsets equi-width
                for(auto iter = offsets.begin(); iter != offsets.end(); ++iter)
                {
                    if(is_positive(iter->width))
                        iter->width = avg_width;
                }
                // set new word_space
                for(auto iter = states.begin(); iter != states.end(); ++iter)
                {
                    double new_word_space = avg_width - iter->single_space_offset() + iter->word_space;

                    // install new word_space
                    // we might introduce more variance here
                    auto & wm = renderer->word_space_manager;
                    wm.install(new_word_space);
                    iter->ids[State::WORD_SPACE_ID] = wm.get_id();
                    iter->word_space = wm.get_actual_value();
                    iter->hash_umask &= (~word_space_umask);
                }
            }
        }
    }
}

// this state will be converted to a child node of the node of prev_state
// dump the difference between previous state
// also clone corresponding states
void HTMLRenderer::TextLineBuffer::State::begin (ostream & out, const State * prev_state)
{
    long long cur_mask = 0xff;
    bool first = true;
    for(int i = 0; i < ID_COUNT; ++i, cur_mask<<=8)
    {
        if(hash_umask & cur_mask) // we don't care about this ID
        {
            if (prev_state && (!(prev_state->hash_umask & cur_mask))) // if prev_state have it set
            {
                // we have to inherit it
                ids[i] = prev_state->ids[i]; 
                hash_umask &= (~cur_mask);
            }
            //anyway we don't have to output it
            continue;
        }

        // now we care about the ID
        if(prev_state && (!(prev_state->hash_umask & cur_mask)) && (prev_state->ids[i] == ids[i]))
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
        out << css_class_names[i];
        if (ids[i] == -1)
            out << CSS::INVALID_ID;
        else
            out << ids[i];
    }

    if(first) // we actually just inherit the whole prev_state
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
    long long common_mask = ~(hash_umask | s.hash_umask);
    if((hash_value & common_mask) == (s.hash_value & common_mask)) return 0;

    long long cur_mask = 0xff;
    int d = 0;
    for(int i = 0; i < ID_COUNT; ++i)
    {
        if((common_mask & cur_mask) && (ids[i] != s.ids[i]))
            ++ d;
        cur_mask <<= 8;
    }
    return d;
}

double HTMLRenderer::TextLineBuffer::State::single_space_offset(void) const
{
    return word_space + letter_space + font_info->space_width * draw_font_size;
}

// the order should be the same as in the enum
const char * const HTMLRenderer::TextLineBuffer::State::css_class_names [] = {
    CSS::FONT_FAMILY_CN,
    CSS::FONT_SIZE_CN,
    CSS::FILL_COLOR_CN,
    CSS::STROKE_COLOR_CN,
    CSS::LETTER_SPACE_CN,
    CSS::WORD_SPACE_CN,
    CSS::RISE_CN
};

} //namespace pdf2htmlEX
