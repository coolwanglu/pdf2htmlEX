/*
 * Constants
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef CONST_H__
#define CONST_H__

#include <map>
#include <string>

namespace pdf2htmlEX {

static const double EPS = 1e-6;
static const double DEFAULT_DPI = 72.0;
extern const double ID_MATRIX[6];

// PDF base 14 font name -> CSS font name
extern const std::map<std::string, std::string> BASE_14_FONT_CSS_FONT_MAP;
// For GB encoded font names
extern const std::map<std::string, std::string> GB_ENCODED_FONT_NAME_MAP;
// map to embed files into html
// key: (suffix, if_embed_content)
// value: (prefix string, suffix string)
extern const std::map<std::pair<std::string, bool>, std::pair<std::string, std::string> > EMBED_STRING_MAP;

} // namespace pdf2htmlEX

#endif //CONST_H__
