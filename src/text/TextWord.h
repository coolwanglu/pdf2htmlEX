// TextWord: a horizontal piece of consective characters and shifts, sharing the same styles
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>

#ifndef TextWord_H__
#define TextWord_H__

#include <vector>

#include <boost/intrusive/list.hpp>

#include "TextChar.h"

namespace pdf2htmlEX {

// for boost::intrusive::list
struct TextWordListTag;
using TextWordBaseHook = boost::intrusive::list_base_hook< tag<TextWordListTag> >

struct TextWord : TextWordBaseHook
{
private:
    std::vector<CharNode> chars;
    // shift of text origin after this word and before the next word
    double shift_after_x = 0.0;
    double shift_after_y = 0.0;
};

using TextWordList = boost::intrusive::list< 
    TextWord, 
    TextWordBaseHook,
    boost::intrusive::constant_time_size<false> 
>;

}

#endif //TextWord_H__
