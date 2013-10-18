/*
 * Encodings used in HTML
 *
 * by WangLu
 * 2013.02.15
 */

#ifndef ENCODING_H__
#define ENCODING_H__

#include <string>
#include <iostream>

#include <CharTypes.h>

namespace pdf2htmlEX {

/*
 * Escape necessary characters, and map Unicode to UTF-8
 */
void writeUnicodes(std::ostream & out, const Unicode * u, int uLen);


/*
 * URL escaping
 */
//void writeURL(std::ostream & out, const std::string & s);

/*
 * JSON escaping
 */
void writeJSON(std::ostream & out, const std::string & s);

/*
 * HTML tag attribute escaping
 */
void writeAttribute(std::ostream & out, const std::string & s);

} // namespace pdf2htmlEX

#endif //ENCODING_H__
