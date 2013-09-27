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

#include "Param.h"

namespace pdf2htmlEX {

#ifndef nullptr
#define nullptr (NULL)
#endif

static const double EPS = 1e-6;
static const double DEFAULT_DPI = 72.0;
extern const double ID_MATRIX[6];

// For GB encoded font names
extern const std::map<std::string, std::string> GB_ENCODED_FONT_NAME_MAP;
// map to embed files into html
struct EmbedStringEntry
{
    int Param::*embed_flag; 
    // used when *embed_flag == true
    std::string prefix_embed;
    std::string suffix_embed;
    bool base64_encode;
    // used when *embed_flag == false
    std::string prefix_external;
    std::string suffix_external;
};
extern const std::map<std::string, EmbedStringEntry> EMBED_STRING_MAP;

extern const std::map<std::string, std::string> FORMAT_MIME_TYPE_MAP;

} // namespace pdf2htmlEX

#endif //CONST_H__
