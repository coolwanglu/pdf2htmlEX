/*
 * HTMLTextLine.cc
 *
 * Generate and optimized HTML for one line
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <cmath>
#include <algorithm>

#include "HTMLTextLine.h"

#include "util/encoding.h"
#include "util/css_const.h"

namespace pdf2htmlEX {

using std::min;
using std::max;
using std::vector;
using std::ostream;
using std::cerr;
using std::endl;
using std::find;
using std::abs;

HTMLTextLine::HTMLTextLine (const HTMLLineState & line_state, const Param & param, AllStateManager & all_manager) 
    :param(param)
    ,all_manager(all_manager) 
    ,line_state(line_state)
    ,clip_x1(0)
    ,clip_y1(0)
    ,width(0)
{ }

void HTMLTextLine::append_unicodes(const Unicode * u, int l, double width)
{
    if (l == 1) 
        text.push_back(min(u[0], (unsigned)INT_MAX));
    else if (l > 1)
    {
        text.push_back(- decomposed_text.size() - 1);
        decomposed_text.emplace_back();
        decomposed_text.back().assign(u, u + l);
    }
    this->width += width;
}

void HTMLTextLine::append_offset(double width)
{
    /*
     * If the last offset is very thin, we can ignore it and directly use it
     * But this should not happen often, and we will also filter near-zero offsets when outputting them
     * So don't check it.
     *
     * Offset must be appended immediately after the last real (non-padding) char, or the text optimizing
     * algorithm may be confused: it may wrongly convert offsets at the beginning of a line to word-space.
     */

    auto offset_idx = text.size();
    while (offset_idx > 0 && text[offset_idx - 1] == 0)
        --offset_idx;
    if((!offsets.empty()) && (offsets.back().start_idx == offset_idx))
        offsets.back().width += width;
    else
        offsets.emplace_back(offset_idx, width);
    this->width += width;
}

void HTMLTextLine::append_state(const HTMLTextState & text_state)
{
    if(states.empty() || (states.back().start_idx != text.size()))
    {
        states.emplace_back();
        states.back().start_idx = text.size();
        states.back().hash_umask = 0;
    }

    HTMLTextState & last_state = states.back();
    last_state = text_state;
    //apply font scale
    last_state.font_size *= last_state.font_info->font_size_scale;
}

void HTMLTextLine::dump_char(std::ostream & out, int pos)
{
    int c = text[pos];
    if (c > 0)
    {
        Unicode u = c;
        writeUnicodes(out, &u, 1);
    }
    else if (c < 0)
    {
        auto dt = decomposed_text[- c - 1];
        writeUnicodes(out, &dt.front(), dt.size());
    }
}

void HTMLTextLine::dump_chars(ostream & out, int begin, int len)
{
    static const Color transparent(0, 0, 0, true);

    if (line_state.first_char_index < 0)
    {
        for (int i = 0; i < len; i++)
            dump_char(out, begin + i);
        return;
    }

    bool invisible_group_open = false;
    for(int i = 0; i < len; i++)
    {
        if (!line_state.is_char_covered(line_state.first_char_index + begin + i)) //visible
        {
            if (invisible_group_open)
            {
                invisible_group_open = false;
                out << "</span>";
            }
            dump_char(out, begin + i);
        }
        else
        {
            if (!invisible_group_open)
            {
                out << "<span class=\"" << all_manager.fill_color.get_css_class_name()
                    << all_manager.fill_color.install(transparent) << " " << all_manager.stroke_color.get_css_class_name()
                    << all_manager.stroke_color.install(transparent) << "\">";
                invisible_group_open = true;
            }
            dump_char(out, begin + i);
        }
    }
    if (invisible_group_open)
        out << "</span>";
}

void HTMLTextLine::dump_text(ostream & out)
{
    /*
     * Each Line is an independent absolute positioned block
     * so even we have a few states or offsets, we may omit them
     */
    if(text.empty())
        return;

    if(states.empty() || (states[0].start_idx != 0))
    {
        cerr << "Warning: text without a style! Must be a bug in pdf2htmlEX" << endl;
        return;
    }

    // Start Output
    {
        // open <div> for the current text line
        out << "<div class=\"" << CSS::LINE_CN
            << " " << CSS::TRANSFORM_MATRIX_CN << all_manager.transform_matrix.install(line_state.transform_matrix)
            << " " << CSS::LEFT_CN             << all_manager.left.install(line_state.x - clip_x1)
            << " " << CSS::HEIGHT_CN           << all_manager.height.install(ascent)
            << " " << CSS::BOTTOM_CN           << all_manager.bottom.install(line_state.y - clip_y1)
            ;
        // it will be closed by the first state
    }

    std::vector<State*> stack;
    // a special safeguard in the bottom
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
            double vertical_align = state_iter1->vertical_align;
            int best_cost = State::HASH_ID_COUNT + 1;
            // we have a nullptr at the beginning, so no need to check for rend
            for(auto iter = stack.rbegin(); *iter; ++iter)
            {
                int cost = state_iter1->diff(**iter);
                if(!equal(vertical_align,0))
                    ++cost;

                if(cost < best_cost)
                {
                    while(stack.back() != *iter)
                    {
                        stack.back()->end(out);
                        stack.pop_back();
                    }
                    best_cost = cost;
                    state_iter1->vertical_align = vertical_align;

                    if(best_cost == 0)
                        break;
                }

                // cannot go further
                if((*iter)->start_idx <= last_text_pos_with_negative_offset)
                    break;

                vertical_align += (*iter)->vertical_align;
            }
            // 
            state_iter1->ids[State::VERTICAL_ALIGN_ID] = all_manager.vertical_align.install(state_iter1->vertical_align);
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
                if(std::abs(target) <= param.h_eps)
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
                        if(std::abs(target - space_off) <= param.h_eps)
                        {
                            Unicode u = ' ';
                            writeUnicodes(out, &u, 1);
                            actual_offset = space_off;
                            done = true;
                        }
                    }

                    // finally, just dump it
                    if(!done)
                    {
                        long long wid = all_manager.whitespace.install(target, &actual_offset);

                        if(!equal(actual_offset, 0))
                        {
                            if(is_positive(-actual_offset))
                                last_text_pos_with_negative_offset = cur_text_idx;

                            double threshold = state_iter1->em_size() * (param.space_threshold);

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
                dump_chars(out, cur_text_idx, next_text_idx - cur_text_idx);
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
}

void HTMLTextLine::clear(void)
{
    states.clear();
    offsets.clear();
    text.clear();
}

void HTMLTextLine::clip(const HTMLClipState & clip_state)
{
    clip_x1 = clip_state.xmin;
    clip_y1 = clip_state.ymin;
}

void HTMLTextLine::prepare(void)
{
    // max_ascent determines the height of the div
    double accum_vertical_align = 0; // accumulated
    ascent = 0;
    descent = 0;
    // note that vertical_align cannot be calculated here
    for(auto iter = states.begin(); iter != states.end(); ++iter)
    {
        auto font_info = iter->font_info;
        iter->ids[State::FONT_ID] = font_info->id;
        iter->ids[State::FONT_SIZE_ID]      = all_manager.font_size.install(iter->font_size);
        iter->ids[State::FILL_COLOR_ID]     = all_manager.fill_color.install(iter->fill_color);
        iter->ids[State::STROKE_COLOR_ID]   = all_manager.stroke_color.install(iter->stroke_color);
        iter->ids[State::LETTER_SPACE_ID]   = all_manager.letter_space.install(iter->letter_space);
        iter->ids[State::WORD_SPACE_ID]     = all_manager.word_space.install(iter->word_space);
        iter->hash();

        accum_vertical_align += iter->vertical_align;
        double cur_ascent = accum_vertical_align + font_info->ascent * iter->font_size;
        if(cur_ascent > ascent)
            ascent = cur_ascent;
        double cur_descent = accum_vertical_align + font_info->descent * iter->font_size;
        if(cur_descent < descent)
            descent = cur_descent;
    }
}


void HTMLTextLine::optimize(std::vector<HTMLTextLine*> & lines)
{
    if(param.optimize_text == 3)
    {
        optimize_aggressive(lines);
    }
    else
    {
        optimize_normal(lines);
    }
}
/*
 * Adjust letter space and word space in order to reduce the number of HTML elements
 * May also unmask word space
 */
void HTMLTextLine::optimize_normal(std::vector<HTMLTextLine*> & lines)
{
    // remove useless states in the end
    while((!states.empty()) && (states.back().start_idx >= text.size()))
        states.pop_back();

    assert(!states.empty());

    const long long word_space_umask = State::umask_by_id(State::WORD_SPACE_ID);

    // for optimization, we need accurate values
    auto & ls_manager = all_manager.letter_space;
    auto & ws_manager = all_manager.word_space;
    
    // statistics of widths
    std::map<double, size_t> width_map;
    // store optimized offsets
    std::vector<Offset> new_offsets;
    new_offsets.reserve(offsets.size());

    auto offset_iter1 = offsets.begin();
    for(auto state_iter1 = states.begin(); state_iter1 != states.end(); ++state_iter1)
    {
        const auto state_iter2 = std::next(state_iter1);
        const size_t text_idx1 = state_iter1->start_idx;
        const size_t text_idx2 = (state_iter2 == states.end()) ? text.size() : state_iter2->start_idx;
        const size_t text_count = text_idx2 - text_idx1;

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
                if((iter != width_map.end()) && (std::abs(iter->first - target) <= EPS))
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

            // negative letter space may cause problems
            if((max_count <= text_count / 2) || (!is_positive(state_iter1->letter_space + most_used_width)))
            { 
                // the old value is the best
                // just copy old offsets
                new_offsets.insert(new_offsets.end(), offset_iter1, offset_iter2);
            }
            else
            {
                // now we would like to adjust letter space to most_used width
                
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
        
        // In some PDF files all spaces are converted into positioning shift
        // We may try to change (some of) them to ' ' by adjusting word_space
        // for now, we consider only the no-space scenario
        // which also includes the case when param.space_as_offset is set

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
                double threshold = (state_iter1->em_size()) * (param.space_threshold);
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

    lines.push_back(this);
}

// for optimize-text == 3
void HTMLTextLine::optimize_aggressive(std::vector<HTMLTextLine*> & lines)
{
    /*
    HTMLLineState original_line_state = line_state;
    // break the line if there are a large (positive or negative) shift
    // letter space / word space are not taken into consideration (yet)
    while(true) 
    {
    }

    // aggressive optimization
    if(target > state_iter1->em_size() * (param.space_threshold) - EPS)
        out << ' ';
    dx = 0;
    lines.push_back(this);
    */
}

// this state will be converted to a child node of the node of prev_state
// dump the difference between previous state
// also clone corresponding states
void HTMLTextLine::State::begin (ostream & out, const State * prev_state)
{
    if(prev_state)
    {
        long long cur_mask = 0xff;
        bool first = true;
        for(int i = 0; i < HASH_ID_COUNT; ++i, cur_mask<<=8)
        {
            if(hash_umask & cur_mask) // we don't care about this ID
            {
                if (prev_state->hash_umask & cur_mask) // if prev_state do not care about it either
                    continue;

                // otherwise
                // we have to inherit it
                ids[i] = prev_state->ids[i]; 
                hash_umask &= (~cur_mask);
                //copy the corresponding value
                //TODO: this is so ugly
                switch(i)
                {
                    case FONT_SIZE_ID:
                        font_size = prev_state->font_size;
                        break;
                    case LETTER_SPACE_ID:
                        letter_space = prev_state->letter_space;
                        break;
                    case WORD_SPACE_ID:
                        word_space = prev_state->word_space;
                        break;
                    default:
                        cerr << "unexpected state mask" << endl;
                        break;
                }
            }

            // now we care about the ID
            
            // if the value from prev_state is the same, we don't need to dump it
            if((!(prev_state->hash_umask & cur_mask)) && (prev_state->ids[i] == ids[i]))
                continue;

            // so we have to dump it
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
        // vertical align
        if(!equal(vertical_align, 0))
        {
            // so we have to dump it
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
            out << CSS::VERTICAL_ALIGN_CN;
            auto id = ids[VERTICAL_ALIGN_ID];
            if (id == -1)
                out << CSS::INVALID_ID;
            else
                out << id;
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
    else
    {
        // prev_state == nullptr
        // which means this is the first state of the line
        // there should be a open pending <div> left there
        // it is not necessary to output vertical align
        long long cur_mask = 0xff;
        for(int i = 0; i < HASH_ID_COUNT; ++i, cur_mask<<=8)
        {
            if(hash_umask & cur_mask) // we don't care about this ID
                continue;

            // now we care about the ID
            out << ' '; 
            // out should have hex set
            out << css_class_names[i];
            if (ids[i] == -1)
                out << CSS::INVALID_ID;
            else
                out << ids[i];
        }

        out << "\">";
        need_close = false;
    }
}

void HTMLTextLine::State::end(ostream & out) const
{
    if(need_close)
        out << "</span>";
}

void HTMLTextLine::State::hash(void)
{
    hash_value = 0;
    for(int i = 0; i < ID_COUNT; ++i)
    {
        hash_value = (hash_value << 8) | (ids[i] & 0xff);
    }
}

int HTMLTextLine::State::diff(const State & s) const
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

long long HTMLTextLine::State::umask_by_id(int id)
{
    return (((long long)0xff) << (8*id));
}

// the order should be the same as in the enum
const char * const HTMLTextLine::State::css_class_names [] = {
    CSS::FONT_FAMILY_CN,
    CSS::FONT_SIZE_CN,
    CSS::FILL_COLOR_CN,
    CSS::STROKE_COLOR_CN,
    CSS::LETTER_SPACE_CN,
    CSS::WORD_SPACE_CN,
    CSS::VERTICAL_ALIGN_CN,
};

} //namespace pdf2htmlEX
