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

void HTMLRenderer::TextLineBuffer::set_pos(GfxState * state)
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
    /*
     * If the last offset is very thin, we can ignore it and directly use it
     * But this should not happen often, and we will also filter near-zero offsets when outputting them
     * So don't check it
     */
    if((!offsets.empty()) && (offsets.back().start_idx == text.size()))
        offsets.back().width += width;
    else
        offsets.emplace_back(text.size(), width);
}

void HTMLRenderer::TextLineBuffer::append_state(void)
{
    if(states.empty() || (states.back().start_idx != text.size()))
    {
        states.emplace_back();
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
    if(text.empty())
    { 
        states.clear();
        offsets.clear();
        return;
    }

    // remove unuseful states in the end
    while((!states.empty()) && (states.back().start_idx >= text.size()))
        states.pop_back();

    if(states.empty() || (states[0].start_idx != 0))
    {
        cerr << "Warning: text without a style! Must be a bug in pdf2htmlEX" << endl;
        text.clear();
        offsets.clear();
        return;
    }

    // optimize before output
    optimize();

    // Start Output
    ostream & out = renderer->f_pages.fs;
    {
        // max_ascent determines the height of the div
        double max_ascent = 0;
        for(auto iter = states.begin(); iter != states.end(); ++iter)
        {
            double cur_ascent = iter->font_info->ascent * iter->draw_font_size;
            if(cur_ascent > max_ascent)
                max_ascent = cur_ascent;
            iter->hash();
        }

        long long hid = renderer->height_manager.install(max_ascent);
        long long lid = renderer->left_manager  .install(x);
        long long bid = renderer->bottom_manager.install(y);

        // open <div> for the current text line
        out << "<div class=\"" << CSS::LINE_CN
            << " "             << CSS::TRANSFORM_MATRIX_CN << tm_id 
            << " "             << CSS::LEFT_CN             << lid
            << " "             << CSS::HEIGHT_CN           << hid
            << " "             << CSS::BOTTOM_CN           << bid
            << "\">";
    }

    // a special safeguard in the bottom
    stack.clear();
    stack.push_back(nullptr);

    //accumulated horizontal offset;
    double dx = 0;

    // whenever a negative offset appears, we should not pop out that <span>
    // otherwise the effect of negative margin-left would disappear
    size_t last_text_pos_with_negative_offset = 0;
    size_t cur_text_idx = 0;

    auto cur_offset_iter = offsets.begin();
    for(auto state_iter2 = states.begin(), state_iter1 = state_iter2++; 
            state_iter1 != states.end(); 
            ++state_iter1, ++state_iter2)
    {
        // export current state, find a closest parent
        { 
            // greedy
            int best_cost = State::ID_COUNT;
            // we have a nullptr at the beginning, so no need to check for rend
            for(auto iter = stack.rbegin(); *iter; ++iter)
            {
                int cost = state_iter1->diff(**iter);
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
            // export the diff between *state_iter1 and stack.back()
            state_iter1->begin(out, stack.back());
            stack.push_back(&*state_iter1);
        }

        // [state_iter1->start_idx, text_idx2) are covered by the current state
        size_t text_idx2 = (state_iter2 == states.end()) ? text.size() : state_iter2->start_idx;

        // dump all text and offsets before next state
        while(true)
        {
            if((cur_offset_iter != offsets.end()) 
                    && (cur_offset_iter->start_idx <= cur_text_idx))
            {
                if(cur_offset_iter->start_idx > text_idx2)
                    break;
                // next is offset
                double target = cur_offset_iter->width + dx;
                double actual_offset = 0;

                //ignore near-zero offsets
                if(abs(target) <= renderer->param->h_eps)
                {
                    actual_offset = 0;
                }
                else
                {
                    bool done = false;
                    // check if the offset is equivalent to a single ' '
                    if(!(state_iter1->hash_umask & State::umask_by_id(State::WORD_SPACE_ID)))
                    {
                        double space_off = state_iter1->single_space_offset();
                        if(abs(target - space_off) <= renderer->param->h_eps)
                        {
                            Unicode u = ' ';
                            outputUnicodes(out, &u, 1);
                            actual_offset = space_off;
                            done = true;
                        }
                    }

                    // finally, just dump it
                    if(!done)
                    {
                        long long wid = renderer->whitespace_manager.install(target, &actual_offset);

                        if(!equal(actual_offset, 0))
                        {
                            if(is_positive(-actual_offset))
                                last_text_pos_with_negative_offset = cur_text_idx;

                            double threshold = state_iter1->em_size() * (renderer->param->space_threshold);

                            out << "<span class=\"" << CSS::WHITESPACE_CN
                                << ' ' << CSS::WHITESPACE_CN << wid << "\">" << (target > (threshold - EPS) ? " " : "") << "</span>";
                        }
                    }
                }
                dx = target - actual_offset;
                ++ cur_offset_iter;
            }
            else
            {
                if(cur_text_idx >= text_idx2)
                    break;
                // next is text
                size_t next_text_idx = text_idx2;
                if((cur_offset_iter != offsets.end()) && (cur_offset_iter->start_idx) < next_text_idx)
                    next_text_idx = cur_offset_iter->start_idx;
                outputUnicodes(out, (&text.front()) + cur_text_idx, next_text_idx - cur_text_idx);
                cur_text_idx = next_text_idx;
            }
        }
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
    // TODO: as letter_space and word_space may be modified (optimization)
    // we should not install them so early
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

/*
 * Adjust letter space and word space in order to reduce the number of HTML elements
 * May also unmask word space
 */
void HTMLRenderer::TextLineBuffer::optimize()
{
    if(!(renderer->param->optimize_text))
        return;

    assert(!states.empty());

    const long long word_space_umask = State::umask_by_id(State::WORD_SPACE_ID);

    // for optimization, we need accurate values
    auto & ls_manager = renderer->letter_space_manager;
    auto & ws_manager = renderer->word_space_manager;
    
    // statistics of widths
    std::map<double, size_t> width_map;
    // store optimized offsets
    std::vector<Offset> new_offsets;
    new_offsets.reserve(offsets.size());

    auto offset_iter1 = offsets.begin();
    for(auto state_iter2 = states.begin(), state_iter1 = state_iter2++; 
            state_iter1 != states.end(); 
            ++state_iter1, ++state_iter2)
    {
        const size_t text_idx1 = state_iter1->start_idx;
        const size_t text_idx2 = (state_iter2 == states.end()) ? text.size() : state_iter2->start_idx;
        size_t text_count = text_idx2 - text_idx1;

        // there might be some offsets before the first state
        while((offset_iter1 != offsets.end()) 
                && (offset_iter1->start_idx <= text_idx1))
        {
            new_offsets.push_back(*(offset_iter1++));
        }

        // find the last offset covered by the current state
        auto offset_iter2 = offset_iter1;
        for(; (offset_iter2 != offsets.end()) && (offset_iter2->start_idx <= text_idx2); ++offset_iter2) { }

        // There are `offset_count` <span>'s, the target is to reduce this number
        size_t offset_count = offset_iter2 - offset_iter1;
        assert(text_count >= offset_count);
        
        // Optimize letter space
        // how much letter_space is changed
        // will be later used for optimizing word space
        double letter_space_diff = 0; 
        width_map.clear();

        // In some PDF files all letter spaces are implemented as position shifts between each letter
        // try to simplify it with a proper letter space
        if(offset_count > 0)
        {
            // mark the current letter_space
            if(text_count > offset_count)
                width_map.insert(std::make_pair(0, text_count - offset_count));

            for(auto off_iter = offset_iter1; off_iter != offset_iter2; ++off_iter)
            {
                const double target = off_iter->width;
                auto iter = width_map.lower_bound(target-EPS);
                if((iter != width_map.end()) && (abs(iter->first - target) <= EPS))
                {
                    ++ iter->second;
                }
                else
                {
                    width_map.insert(iter, std::make_pair(target, 1));
                }
            }
            
            // TODO snapping the widths may result a better result
            // e.g. for (-0.7 0.6 -0.2 0.3 10 10), 0 is better than 10
            double most_used_width = 0;
            size_t max_count = 0;
            for(auto iter = width_map.begin(); iter != width_map.end(); ++iter)
            {
                if(iter->second > max_count)
                {
                    most_used_width = iter->first;
                    max_count = iter->second;
                }
            }

            // now we would like to adjust letter space to most_used width
            // we shall apply the optimization only when it can significantly reduce the number of elements
            if(max_count <= text_count / 2)
            { 
                // the old value is the best
                // just copy old offsets
                new_offsets.insert(new_offsets.end(), offset_iter1, offset_iter2);
            }
            else
            {
                // install new letter space
                const double old_ls = state_iter1->letter_space;
                state_iter1->ids[State::LETTER_SPACE_ID] = ls_manager.install(old_ls + most_used_width, &(state_iter1->letter_space));
                letter_space_diff = old_ls - state_iter1->letter_space;
                // update offsets
                auto off_iter = offset_iter1; 
                // re-count number of offsets
                offset_count = 0;
                for(size_t cur_text_idx = text_idx1; cur_text_idx < text_idx2; ++cur_text_idx)
                {
                    double cur_width = 0;
                    if((off_iter != offset_iter2) && (off_iter->start_idx == cur_text_idx + 1))
                    {
                        cur_width = off_iter->width + letter_space_diff;
                        ++off_iter;
                    }
                    else
                    {
                        cur_width = letter_space_diff ;
                    }
                    if(!equal(cur_width, 0))
                    {
                        new_offsets.emplace_back(cur_text_idx+1, cur_width);
                        ++ offset_count;
                    }
                }
            }
        }

        // Optimize word space
        
        // In some PDF files all spaces are converted into positionig shift
        // We may try to change (some of) them to ' ' by adjusting word_space
        // for now, we cosider only the no-space scenario
        // which also includes the case when param->space_as_offset is set

        // get the text segment covered by current state (*state_iter1)
        const auto text_iter1 = text.begin() + text_idx1;
        const auto text_iter2 = text.begin() + text_idx2;
        if(find(text_iter1, text_iter2, ' ') == text_iter2)
        {
            // if there is not any space, we may change the value of word_space arbitrarily
            // note that we may only change word space, no offset will be affected
            // The actual effect will emerge during flushing, where it could be detected that an offset can be optimized as a single space character
            
            if(offset_count > 0)
            {
                double threshold = (state_iter1->em_size()) * (renderer->param->space_threshold);
                // set word_space for the most frequently used offset
                double most_used_width = 0;
                size_t max_count = 0;

                // if offset_count > 0, we must have updated width_map in the previous step
                // find the most frequent width, with new letter space applied
                for(auto iter = width_map.begin(); iter != width_map.end(); ++iter)
                {
                    double fixed_width = iter->first + letter_space_diff; // this is the actual offset in HTML
                    // we don't want to add spaces for tiny gaps, or even negative shifts
                    if((fixed_width >= threshold - EPS) && (iter->second > max_count))
                    {
                        max_count = iter->second;
                        most_used_width = fixed_width;
                    }
                }

                state_iter1->word_space = 0; // clear word_space for single_space_offset
                double new_word_space = most_used_width - state_iter1->single_space_offset();
                state_iter1->ids[State::WORD_SPACE_ID] = ws_manager.install(new_word_space, &(state_iter1->word_space)); // install new word_space
                state_iter1->hash_umask &= (~word_space_umask); // mark that the word_space is not free
            }
            else // there is no offset at all
            {
                state_iter1->hash_umask |= word_space_umask; // we just free word_space
            }
        }
        offset_iter1 = offset_iter2;
    } 
    
    // apply optimization
    std::swap(offsets, new_offsets);
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
                //copy the corresponding value
                //TODO: this is so ugly
                switch(i)
                {
                case FONT_SIZE_ID:
                    draw_font_size = prev_state->draw_font_size;
                    break;
                case LETTER_SPACE_ID:
                    letter_space = prev_state->letter_space;
                    break;
                case WORD_SPACE_ID:
                    word_space = prev_state->word_space;
                    break;
                default:
                    break;
                }
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

double HTMLRenderer::TextLineBuffer::State::em_size(void) const
{
    return draw_font_size * (font_info->ascent - font_info->descent);
}

long long HTMLRenderer::TextLineBuffer::State::umask_by_id(int id)
{
    return (((long long)0xff) << (8*id));
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
