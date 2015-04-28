// TextSegment: a list of WordNode's on the same horizontal line
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>

#ifndef TEXTSEGMENT_H__
#define TEXTSEGMENT_H__

#include <list>

#include "TextWord.h"
#include "TextStyle.h"

namespace pdf2htmlEX {

struct TextSegment 
{
    using TextWordList = std::list<TextWord>;
    using iterator = TextWordList::iterator;
    iterator begin() { return begin_word; }
    iterator end() { return end_word; }
    bool empty() const { return begin_word == end_word; }
    void clear() { begin_word = end_word; }

private:
    TextStyle style;
    iterator begin_word, end_word;

    friend struct TextPageBuilder;
};

using TextSegmentList = std::list<TextSegment>;

}

#endif //TEXTSEGMENT_H__
