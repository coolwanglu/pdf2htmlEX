/*
 * LineBuffer.cc
 *
 * Generate and optimized HTML for one line
 *
 * by WangLu
 * 2012.09.04
 */

#include <vector>

#include "HTMLRenderer.h"
#include "namespace.h"

namespace pdf2htmlEX {

using std::min;
using std::max;
using std::vector;
using std::ostream;

void HTMLRenderer::LineBuffer::reset(GfxState * state)
{
    state->transform(state->getCurX(), state->getCurY(), &x, &y);
    tm_id = renderer->cur_ttm_id;
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
        offsets.push_back(Offset({text.size(), width}));
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

    HTMLDevice& device = renderer->device;
	device.text_start( x, y, tm_id, renderer->install_height(max_ascent) );

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
            int best_cost = TextState::ID_COUNT;
            
            // we have a nullptr at the beginning, so no need to check for rend
            for(auto iter = stack.rbegin(); *iter; ++iter)
            {
                int cost = cur_state_iter->diff(**iter);
                if(cost < best_cost)
                {
                    while(stack.back() != *iter)
                    {
                        if ( stack.back()->need_close )
							device.span_end();
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
			cur_state_iter->need_close = device.span_start( *cur_state_iter, stack.back() ); 
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

			device.span_whitespace( *stack.back(), target, wid );

            dx = target - w;

            ++ cur_offset_iter;
        }

        size_t next_text_idx = min<size_t>(cur_state_iter->start_idx, cur_offset_iter->start_idx);

        device.text_data( (&text.front()) + cur_text_idx, next_text_idx - cur_text_idx);
        cur_text_idx = next_text_idx;
    }

    // we have a nullptr in the bottom
    while(stack.back())
    {
		if ( stack.back()->need_close )
			device.span_end();
        stack.pop_back();
    }

    device.text_end();


    states.clear();
    offsets.clear();
    text.clear();

}

void HTMLRenderer::LineBuffer::set_state (TextState & state)
{
    state.ids[TextState::FONT_ID] = renderer->cur_font_info->id;
    state.ids[TextState::FONT_SIZE_ID] = renderer->cur_fs_id;
    state.ids[TextState::COLOR_ID] = renderer->cur_color_id;
    state.ids[TextState::LETTER_SPACE_ID] = renderer->cur_ls_id;
    state.ids[TextState::WORD_SPACE_ID] = renderer->cur_ws_id;
    state.ids[TextState::RISE_ID] = renderer->cur_rise_id;

    const FontInfo * info = renderer->cur_font_info;
    state.ascent = info->ascent;
    state.descent = info->descent;
    state.draw_font_size = renderer->draw_font_size;
}


} //namespace pdf2htmlEX
