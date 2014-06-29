/*
 * CoveredTextDetector.cc
 *
 *  Created on: 2014-6-14
 *      Author: duanyao
 */

#include "CoveredTextDetector.h"

#include "util/math.h"

namespace pdf2htmlEX {

void CoveredTextDetector::reset()
{
    char_bboxes.clear();
    chars_covered.clear();
}

void CoveredTextDetector::add_char_bbox(double * bbox)
{
    char_bboxes.insert(char_bboxes.end(), bbox, bbox + 4);
    chars_covered.push_back(false);
}

void CoveredTextDetector::add_char_bbox_clipped(double * bbox, bool patially)
{
    char_bboxes.insert(char_bboxes.end(), bbox, bbox + 4);
    chars_covered.push_back(true);
    if (patially)
        add_non_char_bbox(bbox, chars_covered.size() - 1);
}

void CoveredTextDetector::add_non_char_bbox(double * bbox, int index)
{
    if (index < 0)
        index = chars_covered.size();
    for (int i = 0; i < index; i++)
    {
        if (chars_covered[i])
            continue;
        double * cbbox = &char_bboxes[i * 4];
        if (bbox_intersect(cbbox, bbox))
        {
            chars_covered[i] = true;
            add_non_char_bbox(cbbox, i);
        }
    }
}

}
