/*
 * CoveredTextDetector.cc
 *
 *  Created on: 2014-6-14
 *      Author: duanyao
 */

#include "CoveredTextDetector.h"

#include <algorithm>
#include "util/math.h"

//#define DEBUG

namespace pdf2htmlEX {

CoveredTextDetector::CoveredTextDetector(Param & param): param(param)
{
}

void CoveredTextDetector::reset()
{
    char_bboxes.clear();
    chars_covered.clear();
    char_pts_visible.clear();
}

void CoveredTextDetector::add_char_bbox(cairo_t *cairo, double * bbox)
{
    char_bboxes.insert(char_bboxes.end(), bbox, bbox + 4);
    chars_covered.push_back(false);
    char_pts_visible.push_back(1|2|4|8);
}

void CoveredTextDetector::add_char_bbox_clipped(cairo_t *cairo, double * bbox, int pts_visible)
{
#ifdef DEBUG
    printf("add_char_bbox_clipped: pts_visible:%x: [%f,%f,%f,%f]\n", pts_visible, bbox[0], bbox[1], bbox[2], bbox[3]);
#endif
    char_bboxes.insert(char_bboxes.end(), bbox, bbox + 4);
    char_pts_visible.push_back(pts_visible);

    // DCRH: Hide if no points are visible, or if some points are visible and correct_text_visibility == 2
    if (pts_visible == 0 || param.correct_text_visibility == 2) {
        chars_covered.push_back(true);
        if (pts_visible > 0 && param.correct_text_visibility == 2) {
            param.actual_dpi = std::min(param.text_dpi, param.max_dpi); // Char partially covered so increase background resolution
        }
    } else {
        chars_covered.push_back(false);
    }
}

// We now track the visibility of each corner of the char bbox. Potentially we could track
// more sample points but this should be sufficient for most cases.
// We check to see if each point is covered by any stroke or fill operation
// and mark it as invisible if so
void CoveredTextDetector::add_non_char_bbox(cairo_t *cairo, double * bbox, int what)
{
    int index = chars_covered.size();
    for (int i = 0; i < index; i++) {
        if (chars_covered[i])
            continue;
        double * cbbox = &char_bboxes[i * 4];
#ifdef DEBUG
printf("add_non_char_bbox: what=%d, cbbox:[%f,%f,%f,%f], bbox:[%f,%f,%f,%f]\n", what, cbbox[0], cbbox[1], cbbox[2], cbbox[3], bbox[0], bbox[1], bbox[2], bbox[3]);
#endif
        if (bbox_intersect(cbbox, bbox)) {
            int pts_visible = char_pts_visible[i];
#ifdef DEBUG
printf("pts_visible=%x\n", pts_visible);
#endif
            if ((pts_visible & 1) && cairo_in_clip(cairo, cbbox[0], cbbox[1]) &&
                    (what == 0 ||
                    (what == 1 && cairo_in_fill(cairo, cbbox[0], cbbox[1])) ||
                    (what == 2 && cairo_in_stroke(cairo, cbbox[0], cbbox[1])))) {
                    pts_visible &= ~1;
            }
            if ((pts_visible & 2) && cairo_in_clip(cairo, cbbox[2], cbbox[1]) &&
                    (what == 0 ||
                    (what == 1 && cairo_in_fill(cairo, cbbox[2], cbbox[1])) ||
                    (what == 2 && cairo_in_stroke(cairo, cbbox[2], cbbox[1])))) {
                    pts_visible &= ~2;
            }
            if ((pts_visible & 4) && cairo_in_clip(cairo, cbbox[2], cbbox[3]) &&
                    (what == 0 ||
                    (what == 1 && cairo_in_fill(cairo, cbbox[2], cbbox[3])) ||
                    (what == 2 && cairo_in_stroke(cairo, cbbox[2], cbbox[3])))) {
                    pts_visible &= ~4;
            }
            if ((pts_visible & 8) && cairo_in_clip(cairo, cbbox[0], cbbox[3]) &&
                    (what == 0 ||
                    (what == 1 && cairo_in_fill(cairo, cbbox[0], cbbox[3])) ||
                    (what == 2 && cairo_in_stroke(cairo, cbbox[0], cbbox[3])))) {
                    pts_visible &= ~8;
            }
#ifdef DEBUG
printf("pts_visible=%x\n", pts_visible);
#endif
            char_pts_visible[i] = pts_visible;
            if (pts_visible == 0 || (pts_visible != (1|2|4|8) && param.correct_text_visibility == 2)) {
#ifdef DEBUG
printf("Char covered\n");
#endif
                chars_covered[i] = true;
                if (pts_visible > 0 && param.correct_text_visibility == 2) { // Partially visible text => increase rendering DPI
                    param.actual_dpi = std::min(param.text_dpi, param.max_dpi);
                }
            }
        } else {
#ifdef DEBUG
printf("Not covered\n");
#endif
        }
    }
}

}
