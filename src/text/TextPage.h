// TextPage: a list of segments
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>

#ifndef TEXTPAGE_H__
#define TEXTPAGE_H__

#include "TextSegment.h"

namespace pdf2htmlEX {

struct TextPage 
{
    ~TextPage() { segments.clear_and_dispose([](void *p){ delete p; }); }

    const double * get_default_ctm () const { return default_ctm; }

    using iterator = TextSegmentList::iterator;
    iterator begin() { return segments.begin(); }
    iterator end() { return segments.end(); }
    void push_back(TextSegment& segment) { segments.push_back(segment); }

private:
    TextSegmentList segments;

    int num;
    double width, height;
    double default_ctm[6];
};

}

#endif //TEXTPAGE_H__
