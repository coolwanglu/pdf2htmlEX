/*
 * Constants
 *
 * by WangLu
 * 2012.11.29
 */

#include "const.h"

namespace pdf2htmlEX {

using std::map;
using std::string;

const double ID_MATRIX[6] = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};

const map<string, string> GB_ENCODED_FONT_NAME_MAP({
    {"\xCB\xCE\xCC\xE5", "SimSun"},
    {"\xBA\xDA\xCC\xE5", "SimHei"},
    {"\xBF\xAC\xCC\xE5_GB2312", "SimKai"},
    {"\xB7\xC2\xCB\xCE_GB2312", "SimFang"},
    {"\xC1\xA5\xCA\xE9", "SimLi"},
});

const std::map<std::string, EmbedStringEntry> EMBED_STRING_MAP({
    {".css", {&Param::embed_css, 
              "<style type=\"text/css\">", 
              "</style>",
              "<link rel=\"stylesheet\" type=\"text/css\" href=\"", 
              "\"/>" }},
    {".js", {&Param::embed_javascript,
             "<script type=\"text/javascript\">", 
             "</script>",
             "<script type=\"text/javascript\" src=\"",
             "\"></script>" }}
});

const std::map<std::string, std::string> FORMAT_MIME_TYPE_MAP({
    {"eot", "application/vnd.ms-fontobject"},
    {"jpg", "image/jpeg"},
    {"otf", "appilcation/x-font-otf"},
    {"png", "image/png"},
    {"svg", "image/svg+xml"},
    {"ttf", "application/x-font-ttf"},
    {"woff", "application/font-woff"},
});

} //namespace pdf2htmlEX
