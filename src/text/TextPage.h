// TextPage: a list of segments
// Copyright (C) 2015 Lu Wang <coolwanglu@gmail.com>

#ifndef TEXTPAGE_H__
#define TEXTPAGE_H__

#include "TextSegment.h"

namespace pdf2htmlEX {

struct TextPage 
{
    const double * get_default_ctm () const { return default_ctm; }

private:
    TextCharList chars;
    TextWordList words;
    TextSegmentList segments;

    int num;
    double width, height;
    double default_ctm[6];

    friend struct TextPageBuilder;
};

}

#endif //TEXTPAGE_H__
