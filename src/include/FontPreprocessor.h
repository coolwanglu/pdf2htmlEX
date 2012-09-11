/*
 * FontPreprocessor.h
 *
 * Check used codes for each font
 *
 * by WangLu
 * 2012.09.07
 */


#ifndef FONTPREPROCESSOR_H__
#define FONTPREPROCESSOR_H__

#include <unordered_map>

#include <OutputDev.h>

namespace pdf2htmlEX {

class FontPreprocessor : public OutputDev {
public:
    FontPreprocessor(void);
    virtual ~FontPreprocessor(void);

    virtual GBool upsideDown() { return gFalse; }
    virtual GBool useDrawChar() { return gTrue; }
    virtual GBool interpretType3Chars() { return gFalse; }
    virtual GBool needNonText() { return gFalse; }
    virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);

    const char * get_code_map (long long font_id) const;

protected:
    long long cur_font_id;
    char * cur_code_map;

    std::unordered_map<long long, char*> code_maps;
};

} // namespace pdf2htmlEX

#endif //FONTPREPROCESSOR_H__
