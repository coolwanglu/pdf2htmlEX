/*
 * CSSClassNames.h
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef CSSCLASSNAMES_H__
#define CSSCLASSNAMES_H__


/*
 * should be consistent with base.css and pdf2htmlEX.js
 */

namespace pdf2htmlEX {
namespace CSS {



// usually the class name is XXX_CN or XXX_CN<hex id>
// sometimes we need a special one, e.g. transparent color, where the id is -1
const char * const INVALID_ID          = "_";
// work around strings
// TODOsince we have this string, should this file be named as general "css.h" ?
const char * const WEBKIT_ONLY         = "@media screen and (-webkit-min-device-pixel-ratio:0)";


// TODO: better names, remove collission (i.e LINE_CN vs LETTER_SPACE_CN)
const char * const LINE_CN             = "l";
const char * const TRANSFORM_MATRIX_CN = "t";

// page_decoration is for shadow etc
// page_frame cannot have margin or border-width, pdf2htmlEX.js will use it to determine the coordinates
// page_content holds everything inside the page, could be hidden to speed up rendering
// page_data holds data for pdf2htmlEX.js
const char * const PAGE_DECORATION_CN  = "d";
const char * const PAGE_FRAME_CN       = "p";
const char * const PAGE_CONTENT_BOX_CN = "b";
const char * const PAGE_DATA_CN        = "j";

const char * const FONT_NAME_CN        = "f";
const char * const FONT_SIZE_CN        = "s";
const char * const FILL_COLOR_CN       = "c";
const char * const STROKE_COLOR_CN     = "C";
const char * const LETTER_SPACE_CN     = "l";
const char * const WORD_SPACE_CN       = "w";
const char * const RISE_CN             = "r";
const char * const WHITESPACE_CN       = "_";
const char * const LEFT_CN             = "L";
const char * const HEIGHT_CN           = "h";
const char * const WIDTH_CN            = "W";
const char * const BOTTOM_CN           = "B";

const char * const CSS_DRAW_CN         = "Cd";
const char * const LINK_CN             = "a";

}
}


#endif //CSSCLASSNAMES_H__
