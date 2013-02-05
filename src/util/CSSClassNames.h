/*
 * CSSClassNames.h
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef CSSCLASSNAMES_H__
#define CSSCLASSNAMES_H__

namespace pdf2htmlEX {
namespace CSS {

const char * FONT_SIZE_CN    = "s";
const char * LETTER_SPACE_CN = "l";
const char * WORD_SPACE_CN   = "w";
const char * RISE_CN         = "r";
const char * WHITESPACE_CN   = "_";
const char * HEIGHT_CN       = "h";
const char * LEFT_CN         = "L";

}
}

/*
 * Deprecated!!!
 * Naming Convention
 *
 * CSS classes
 *
 * _ - white space
 * a - Annot link
 * b - page Box
 * d - page Decoration
 * l - Line
 * j - Js data
 * p - Page
 *
 * Cd - CSS Draw
 *
 * Numbered CSS classes
 * See also: HTMLRenderer::TextLineBuffer::format_str
 *
 * t<hex> - Transform matrix
 * f<hex> - Font (also for font names)
 * s<hex> - font Size
 * l<hex> - Letter spacing
 * w<hex> - Word spacing
 * c<hex> - Fill Color
 * C<hex> - Stroke Color
 * _<hex> - white space
 * r<hex> - Rise
 * h<hex> - Height
 * L<hex> - Left
 *
 */


#endif //CSSCLASSNAMES_H__
