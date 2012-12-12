/*
 * Preprocessor.h
 *
 * PDF is so complicated that we have to scan twice
 *
 * Check used codes for each font
 * Collect all used link destinations
 *
 * by WangLu
 * 2012.09.07
 */


#ifndef PREPROCESSOR_H__
#define PREPROCESSOR_H__

#include <unordered_map>

#include <OutputDev.h>
#include <PDFDoc.h>
#include <Annot.h>
#include "Param.h"
#include <vector>

namespace pdf2htmlEX {

using std::vector;

class Preprocessor : public OutputDev {
public:
    Preprocessor(const Param * param);
    virtual ~Preprocessor(void);

    void process(PDFDoc * doc);

    virtual GBool upsideDown() { return gFalse; }
    virtual GBool useDrawChar() { return gTrue; }
    virtual GBool interpretType3Chars() { return gFalse; }
    virtual GBool needNonText() { return gFalse; }

    virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);

    virtual void startPage(int pageNum, GfxState *state);

    const char * get_code_map (long long font_id) const;
    double get_max_width (void) const { return max_width; }
    double get_max_height (void) const { return max_height; }
    double get_page_width (int page_number) { return page_widths.at(page_number-1); }
    double get_page_height (int page_number) { return page_heights.at(page_number-1); }

protected:
    const Param * param;

    double max_width, max_height;
    vector<int> page_widths, page_heights;    

    long long cur_font_id;
    char * cur_code_map;

    std::unordered_map<long long, char*> code_maps;
};

} // namespace pdf2htmlEX

#endif //PREPROCESSOR_H__
