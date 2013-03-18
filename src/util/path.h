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
 * Sanitize all occurrences of '%' except for the first valid format specifier. Filename
 * is only sanitized if a formatter is found, and the function returns true.
 *
 * @param filename the filename to be sanitized. Value will be modified.
 *
 * @return true if a format specifier was found, false otherwise.
 */ 
bool sanitize_filename(std::string & filename);

} //namespace pdf2htmlEX 
#endif //PATH_H__
