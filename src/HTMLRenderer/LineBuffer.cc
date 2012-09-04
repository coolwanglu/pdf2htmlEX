/*
 * LineBuffer.cc
 *
 * Generate and optimized HTML for one line
 *
 * by WangLu
 * 2012.09.04
 */

#include "HTMLRenderer.h"
#include "HTMLRenderer/namespace.h"

using std::min;
using std::max;
using std::hex;
using std::dec;

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

    states.resize(states.size() + 1);
    states.back().start_idx = text.size();

    offsets.push_back({text.size(), 0});

    // TODO: optimize state
    double max_ascent = 0;
    for(const State & s : states)
        max_ascent = max(max_ascent, s.ascent);

    // TODO: class for height ?
    ostream & out = renderer->html_fout;
    out << format("<div style=\"left:%1%px;bottom:%2%px;height:%3%px;\" class=\"l t%|4$x|\">")
        % x % y
        % max_ascent
        % tm_id
        ;

    auto cur_state_iter = states.begin();
    auto cur_offset_iter = offsets.begin();

    double dx = 0;

    size_t cur_text_idx = 0;
    while(cur_text_idx < text.size())
    {
        if(cur_text_idx >= cur_state_iter->start_idx)
        {
            if(cur_text_idx)
                State::end(out);

            cur_state_iter->begin(out);

            ++ cur_state_iter;
        }

        if(cur_text_idx >= cur_offset_iter->start_idx)
        {
            double target = cur_offset_iter->width + dx;
            double w;

            auto wid = renderer->install_whitespace(target, w);

            // TODO
//            double threshold = draw_font_size * (cur_font_info.ascent - cur_font_info.descent) * (param->space_threshold);
            double threshold = 0;
            out << format("<span class=\"_ _%|1$x|\">%2%</span>") % wid % (target > (threshold - EPS) ? " " : "");

            dx = target - w;

            ++ cur_offset_iter;
        }

        size_t next_text_idx = min(cur_state_iter->start_idx, cur_offset_iter->start_idx);
        outputUnicodes(out, text.data() + cur_text_idx, next_text_idx - cur_text_idx);
        cur_text_idx = next_text_idx;
    }

    State::end(out);
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

    state.ascent = renderer->cur_font_info->ascent * renderer->draw_font_size;
}

void HTMLRenderer::LineBuffer::State::begin (ostream & out) const
{
    out << "<span class=\"";
    for(int i = 0; i < ID_COUNT; ++i)
    {
        if(i > 0) out << ' ';
        out << format("%1%%|2$x|") % format_str[i] % ids[i];
    }
    out << "\">";
}

void HTMLRenderer::LineBuffer::State::end(ostream & out)
{
    out << "</span>";
}

const char * HTMLRenderer::LineBuffer::State::format_str = "fsclwr";
