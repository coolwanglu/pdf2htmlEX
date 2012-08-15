/*
 * Constants
 *
 * by WangLu
 * 2012.08.07
 */

#ifndef CONSTS_H__
#define CONSTS_H__

#include <string>
#include <map>

static constexpr double EPS = 1e-6;
extern const double id_matrix[6];

static constexpr double DEFAULT_DPI = 72.0;

extern const std::map<std::string, std::string> BASE_14_FONT_CSS_FONT_MAP;
extern const std::map<std::string, std::string> GB_ENCODED_FONT_NAME_MAP;

#endif //CONSTS_H__
