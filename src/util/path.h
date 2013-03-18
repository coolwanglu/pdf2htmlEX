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
 * Function to sanitize a filename so that it can be eventually safely used in a printf 
 * statement. Allows a single %d placeholder, but no other format specifiers.
 *
 * @param filename the filename to be sanitized.
 *
 * @return the sanitized filename.
 */ 
std::string sanitize_filename(const std::string & filename);

/**
 * Function to check if a filename contains at least one %d integer placeholder
 * for use in a printf statement.
 *
 * @param filename the filename to check
 *
 * @return true if the filename contains an integer placeholder, false otherwise.
 */
bool contains_integer_placeholder(const std::string & filename);

} //namespace pdf2htmlEX 
#endif //PATH_H__
