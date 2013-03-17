/*
 * Function handling filenames and paths
 *
 * by WangLu
 * 2012.11.29
 */

#ifndef PATH_H__
#define PATH_H__

#include <string>

namespace pdf2htmlEX {

void create_directories(const std::string & path);

bool is_truetype_suffix(const std::string & suffix);

std::string get_filename(const std::string & path);
std::string get_suffix(const std::string & path);

/**
 * Function to sanitize a filename so that it can be eventually safely used in a printf statement.
 *
 * @param filename the filename to be sanitized.
 * @param allow_single_form_number boolean flag indicatin if a single format (e.g. %d) should be allowed
 *     in the filename for use in templating of pages. e.g. page%02d.html is ok.
 *
 * @return the sanitized filename.
 */ 
std::string sanitize_filename(const std::string & filename, bool allow_single_format_number);

} //namespace pdf2htmlEX 
#endif //PATH_H__
