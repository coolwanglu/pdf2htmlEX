/*
 * install.h
 *
 * 
 *
 * implemented by WangLu
 * splitted of by Filodej <philode@gmail.com>
 */

#include <iostream>
#include <string>

#include <GfxState.h>

namespace pdf2htmlEX 
{

void export_remote_default_font(std::ostream& out, long long fn_id); 
void export_local_font(std::ostream& out, const FontInfo & info, GfxFont * font, const string & original_font_name, const string & cssfont);
void export_transform_matrix (std::ostream& out, long long tm_id, const double * tm) ;
void export_color (std::ostream& out, long long color_id, const GfxRGB * rgb); 
void export_font_size(std::ostream& out, long long fs_id, double font_size);
void export_letter_space(std::ostream& out, long long ls_id, double letter_space);
void export_word_space(std::ostream& out, long long ws_id, double word_space);
void export_whitespace(std::ostream& out, long long ws_id, double ws_width);
void export_rise(std::ostream& out, long long rise_id, double rise) ;
void export_height(std::ostream& out, long long height_id, double height);


} // namespace pdf2htmlEX
