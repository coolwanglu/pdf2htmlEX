/*
 * CoveredTextDetector.h
 *
 *  Created on: 2014-6-14
 *      Author: duanyao
 */

#ifndef COVEREDTEXTDETECTOR_H__
#define COVEREDTEXTDETECTOR_H__

#include <vector>

namespace pdf2htmlEX {

/**
 * Detect characters that are covered by non-char graphics on a page.
 */
class CoveredTextDetector
{
public:

    /**
     * Reset to initial state. Should be called when start drawing a page.
     */
    void reset();

    /**
     * Add a drawn character's bounding box.
     * @param bbox (x0, y0, x1, y1)
     */
    void add_char_bbox(double * bbox);

    void add_char_bbox_clipped(double * bbox, bool patially);

    /**
     * Add a drawn non-char graphics' bounding box.
     * If it intersects any previously drawn char's bbox, the char is marked as covered
     * and treated as an non-char.
     * @param bbox (x0, y0, x1, y1)
     * @param index this graphics' drawing order: assume it is drawn after (index-1)th
     *   char. -1 means after the last char.
     */
    void add_non_char_bbox(double * bbox, int index = -1);

    /**
     * An array of flags indicating whether a char is covered by any non-char graphics.
     * Index by the order that these chars are added.
     * This vector grows as add_char_bbox() is called, so its size is the count
     * of currently drawn chars.
     */
    const std::vector<bool> & get_chars_covered() { return chars_covered; }

private:
    std::vector<bool> chars_covered;
    // x00, y00, x01, y01; x10, y10, x11, y11;...
    std::vector<double> char_bboxes;
};

}

#endif /* COVEREDTEXTDETECTOR_H__ */
