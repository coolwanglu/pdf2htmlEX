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

const map<string, string> BASE_14_FONT_CSS_FONT_MAP({
   { "Courier", "Courier,monospace" },
   { "Helvetica", "Helvetica,Arial,\"Nimbus Sans L\",sans-serif" },
   { "Times", "Times,\"Time New Roman\",\"Nimbus Roman No9 L\",serif" },
   { "Symbol", "Symbol,\"Standard Symbols L\"" },
   { "ZapfDingbats", "ZapfDingbats,\"Dingbats\"" },
});

const map<string, string> GB_ENCODED_FONT_NAME_MAP({
    {"\xCB\xCE\xCC\xE5", "SimSun"},
    {"\xBA\xDA\xCC\xE5", "SimHei"},
    {"\xBF\xAC\xCC\xE5_GB2312", "SimKai"},
    {"\xB7\xC2\xCB\xCE_GB2312", "SimFang"},
    {"\xC1\xA5\xCA\xE9", "SimLi"},
});

const std::map<std::pair<std::string, bool>, std::pair<std::string, std::string> > EMBED_STRING_MAP({
        {{".css", 0}, {"<link rel=\"stylesheet\" type=\"text/css\" href=\"", "\"/>"}},
        {{".css", 1}, {"<style type=\"text/css\">", "</style>"}},
        {{".js", 0}, {"<script type=\"text/javascript\" src=\"", "\"></script>"}},
        {{".js", 1}, {"<script type=\"text/javascript\">", "</script>"}}
});
} //namespace pdf2htmlEX
