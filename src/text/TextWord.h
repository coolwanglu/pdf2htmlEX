// TextWord: a horizontal piece of consective characters and shifts, sharing the same styles
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>

#ifndef TextWord_H__
#define TextWord_H__

#include <vector>
#include <list>

#include "TextStyle.h"
#include "TextChar.h"

namespace pdf2htmlEX {

struct TextWord
{
    using TextCharList = std::vector<TextChar*>;
    using iterator = TextCharList::iterator;
    iterator begin() { return chars.begin(); }
    iterator end() { return chars.end(); }
    bool empty() const { return chars.empty(); }

private:
    TextStyle style;
    TextCharList chars;
    // shift of text origin after this word and before the next word
    double shift_after_x = 0.0;
    double shift_after_y = 0.0;

    friend struct TextPageBuilder;
};

}

#endif //TextWord_H__
