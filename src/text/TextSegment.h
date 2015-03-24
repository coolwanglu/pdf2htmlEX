// TextSegment: a list of WordNode's on the same horizontal line
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>

#ifndef TEXTSEGMENT_H__
#define TEXTSEGMENT_H__

#include <boost/intrusive/list.hpp>

#include "TextWord.h"
#include "TextStyle.h"

namespace pdf2htmlEX {

// for boost::intrusive::list
struct TextSegmentListTag;
using TextSegmentBaseHook = boost::intrusive::list_base_hook< tag<SegmentListTag> >;

struct TextSegment : TextSegmentBaseHook
{
    ~TextSegment() { words.clear_and_dispose([](void *p){ delete p; }); }

    using iterator = TextWordList::iterator;
    iterator begin() { return words.begin(); }
    iterator end() { return words.end(); }
    bool empty() const { return words.empty();

private:
    TextStyle style;
    TextWordList words;
};

using TextSegmentList = boost::intrusive::list< 
    TextSegment, 
    TextSegmentBaseHook,
    boost::intrusive::constant_time_size<false> 
>;

}

#endif //TEXTSEGMENT_H__
