/*
 * config.h
 * Compile time constants
 *
 * by WangLu
 */


#ifndef PDF2HTMLEX_CONFIG_H__
#define PDF2HTMLEX_CONFIG_H__

#include <string>

#define HAVE_CAIRO 1

namespace pdf2htmlEX {

static const std::string PDF2HTMLEX_VERSION = "0.6";
static const std::string PDF2HTMLEX_PREFIX = "/usr/local";
static const std::string PDF2HTMLEX_DATA_PATH = "/usr/local""/share/pdf2htmlEX";

} // namespace pdf2htmlEX

#endif //PDF2HTMLEX_CONFIG_H__
