#include "TextState.h"
#include <iostream>

using namespace std;

namespace pdf2htmlEX {

const char * TextState::format_str = "fsclwr";

void TextState::begin( ostream & out, const TextState * prev_state )
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
        // out should has set hex
        out << format_str[i] << ids[i];
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

void TextState::end(ostream & out) const
{
	if(need_close)
		out << "</span>";
}

void TextState::hash(void)
{
    hash_value = 0;
    for(int i = 0; i < ID_COUNT; ++i)
    {
        hash_value = (hash_value << 8) | (ids[i] & 0xff);
    }
}

int TextState::diff( TextState const& s) const
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

} // namespace pdf2htmlEX
