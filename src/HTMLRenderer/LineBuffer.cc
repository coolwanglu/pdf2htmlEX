/*
 * LineBuffer.cc
 *
 * Generate and optimized HTML for one line
 *
 * by WangLu
 * 2012.09.04
 */

#include <vector>
#include <stack>

#include "HTMLRenderer.h"
#include "HTMLRenderer/namespace.h"

using std::min;
using std::max;
using std::vector;
using std::stack;
using std::function;

void HTMLRenderer::LineBuffer::reset(GfxState * state)
{
    state->transform(state->getCurX(), state->getCurY(), &x, &y);
    tm_id = renderer->cur_tm_id;
}

void HTMLRenderer::LineBuffer::append_unicodes(const Unicode * u, int l)
{
    text.insert(text.end(), u, u+l);
}

void HTMLRenderer::LineBuffer::append_offset(double width)
{
    if((!offsets.empty()) && (offsets.back().start_idx == text.size()))
        offsets.back().width += width;
    else
        offsets.push_back({text.size(), width});
}

void HTMLRenderer::LineBuffer::append_state(void)
{
    if(states.empty() || (states.back().start_idx != text.size()))
    {
        states.resize(states.size() + 1);
        states.back().start_idx = text.size();
    }

    set_state(states.back());
}

void HTMLRenderer::LineBuffer::flush(void)
{
    /*
     * Each Line is an independent absolute positioined block
     * so even we have a few states or offsets, we may omit them
     */
    if(text.empty()) return;

    if(states.empty() || (states[0].start_idx != 0))
    {
        cerr << "Warning: text without a style! Must be a bug in pdf2htmlEX" << endl;
        return;
    }

    for(auto & s : states)
        s.hash();

    if((renderer->param->optimize) && (states.size() > 2))
    {
        optimize_states();
    }
    else
    {
        for(size_t i = 0; i < states.size(); ++i)
            states[i].depth = i;
    }

    states.resize(states.size() + 1);
    states.back().start_idx = text.size();
    states.back().depth = 0;

    offsets.push_back({text.size(), 0});

    double max_ascent = 0;
    for(const State & s : states)
        max_ascent = max(max_ascent, s.ascent * s.draw_font_size);

    // TODO: class for height ?
    ostream & out = renderer->html_fout;
    out << format("<div style=\"left:%1%px;bottom:%2%px;height:%3%px;\" class=\"l t%|4$x|\">")
        % x % y
        % max_ascent
        % tm_id
        ;

    auto cur_state_iter = states.begin();
    auto cur_offset_iter = offsets.begin();

    //accumulated horizontal offset;
    double dx = 0;

    stack<State*> stack;
    stack.push(nullptr);
    int last_depth = -1;

    size_t cur_text_idx = 0;
    while(cur_text_idx < text.size())
    {
        if(cur_text_idx >= cur_state_iter->start_idx)
        {
            int depth = cur_state_iter -> depth;
            int cnt = last_depth + 1 - depth;
            assert(cnt >= 0);
            while(cnt--)
            {
                stack.top()->end(out);
                stack.pop();
            }

            cur_state_iter->begin(out, stack.top());
            stack.push(&*cur_state_iter);
            last_depth = depth;

            ++ cur_state_iter;
        }

        if(cur_text_idx >= cur_offset_iter->start_idx)
        {
            double target = cur_offset_iter->width + dx;
            double w;

            auto wid = renderer->install_whitespace(target, w);

            double threshold = cur_state_iter->draw_font_size * (cur_state_iter->ascent - cur_state_iter->descent) * (renderer->param->space_threshold);
            out << format("<span class=\"_ _%|1$x|\">%2%</span>") % wid % (target > (threshold - EPS) ? " " : "");

            dx = target - w;

            ++ cur_offset_iter;
        }

        size_t next_text_idx = min(cur_state_iter->start_idx, cur_offset_iter->start_idx);
        outputUnicodes(out, text.data() + cur_text_idx, next_text_idx - cur_text_idx);
        cur_text_idx = next_text_idx;
    }

    // we have a nullptr in the bottom
    while(stack.top())
    {
        stack.top()->end(out);
        stack.pop();
    }

    out << "</div>";


    states.clear();
    offsets.clear();
    text.clear();

}

void HTMLRenderer::LineBuffer::set_state (State & state)
{
    state.ids[State::FONT_ID] = renderer->cur_font_info->id;
    state.ids[State::FONT_SIZE_ID] = renderer->cur_fs_id;
    state.ids[State::COLOR_ID] = renderer->cur_color_id;
    state.ids[State::LETTER_SPACE_ID] = renderer->cur_ls_id;
    state.ids[State::WORD_SPACE_ID] = renderer->cur_ws_id;
    state.ids[State::RISE_ID] = renderer->cur_rise_id;

    const FontInfo * info = renderer->cur_font_info;
    state.ascent = info->ascent;
    state.descent = info->descent;
    state.draw_font_size = renderer->draw_font_size;
}

class DPBufferEntry
{
public:
    int last_child;
    int min_cost;
};

static vector<DPBufferEntry> flattened_dp_buffer;
static vector<DPBufferEntry*> dp_buffer;

void HTMLRenderer::LineBuffer::optimize_states (void)
{
    int n = states.size();

    flattened_dp_buffer.resize(n*(n+1)/2);
    dp_buffer.resize(n);
    
    {
        int incre = n;
        auto iter = dp_buffer.begin();
        DPBufferEntry * p = flattened_dp_buffer.data();
        while(incre > 0)
        {
            *(iter++) = p;
            p += (incre--);
        }
    }

    // depth 0
    for(int i = 0; i < n; ++i)
        flattened_dp_buffer[i].min_cost = 0;
    
    int last_at_this_depth = n;
    for(int depth = 1; depth < n; ++depth)
    {
        --last_at_this_depth;
        for(int i = 0; i < last_at_this_depth; ++i)
        {
            //determine dp_buffer[depth][i]
            int best_last_child = i+1;   
            int best_min_cost = states[i].diff(states[i+1]) + dp_buffer[depth-1][i+1].min_cost;
            // at depth, we consider [i+1, i+depth+1) as possible children of i
            for(int j = 2; j <= depth; ++j)
            {
                int cost = dp_buffer[j-1][i].min_cost + dp_buffer[depth-j][i+j].min_cost;
                // avoid calling diff() when possible
                if (cost >= best_min_cost) continue;

                cost += states[i].diff(states[i+j]);

                if(cost < best_min_cost)
                {
                    best_last_child = i+j;
                    best_min_cost = cost;
                }
            }

            dp_buffer[depth][i] = {best_last_child, best_min_cost};
        }
    }

    // now fill in the depths
    // use recursion for now, until someone finds a PDF that would causes this overflow
    function<void(int,int,int)> func = [&](int idx, int depth, int tree_depth) -> void {
        states[idx].depth = tree_depth;
        while(depth > 0)
        {
            int last_child = dp_buffer[depth][idx].last_child;
            assert((last_child > idx) && (last_child <= idx + depth));
            func(last_child, idx + depth - last_child, tree_depth + 1);
            depth = last_child - idx - 1;
        }
    };

    func(0, n-1, 0);
}

void HTMLRenderer::LineBuffer::State::begin (ostream & out, const State * prev_state)
{
    if(prev_state && (prev_state->hash_value == hash_value))
    {
        // check ids again
        int i;
        for(i = 0; i < ID_COUNT; ++i)
            if(ids[i] != prev_state->ids[i])
                break;
            
        if(i == ID_COUNT)
        {
            need_close = false;
            return;
        }
    }

    need_close = true;

    out << "<span class=\"";
    bool first = true;
    for(int i = 0; i < ID_COUNT; ++i)
    {
        if(prev_state && (prev_state->ids[i] == ids[i]))
            continue;

        if(first)
        { 
            first = false;
        }
        else
        {
            out << ' ';
        }

        out << format("%1%%|2$x|") % format_str[i] % ids[i];
    }

    out << "\">";
}

void HTMLRenderer::LineBuffer::State::end(ostream & out) const
{
    if(need_close)
        out << "</span>";
}

void HTMLRenderer::LineBuffer::State::hash(void)
{
    hash_value = 0;
    for(int i = 0; i < ID_COUNT; ++i)
    {
        hash_value = (hash_value << 8) | (ids[i] & 0xff);
    }
}

int HTMLRenderer::LineBuffer::State::diff(const State & s) const
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

const char * HTMLRenderer::LineBuffer::State::format_str = "fsclwr";
