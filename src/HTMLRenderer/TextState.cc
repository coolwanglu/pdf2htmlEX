#include "TextState.h"
#include <iostream>

using namespace std;

namespace pdf2htmlEX {

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
