/*
 * Parameters
 *
 * Wang Lu
 * 2012.08.03
 */


#ifndef PARAM_H__
#define PARAM_H__

#include <string>

namespace pdf2htmlEX {

struct Param
{
    // PDF stuff
    std::string owner_password, user_password;
    std::string input_filename, output_filename;
    bool no_drm;

    // path
    std::string dest_dir, tmp_dir, data_dir;

    // normal parameters
    int first_page, last_page;

    double zoom;
    double fit_width, fit_height;
    double h_dpi, v_dpi;
    int use_cropbox;

    int process_nontext;
    int single_html;
    int split_pages;
    int embed_base_font;
    int embed_external_font;
    int decompose_ligature;

    // Advanced tweak
    /*
     * Position & Size
     */
    double h_eps, v_eps;
    double space_threshold;
    double font_size_multiplier;

    /*
     * Font
     */
    int auto_hint;
    int tounicode;
    int space_as_offset;
    int stretch_narrow_glyph;
    int squeeze_wide_glyph;
    int remove_unused_glyph;

    std::string font_suffix, font_format;
    std::string external_hint_tool;

    /*
     * Output
     */
    std::string css_filename;

    /*
     * Debug
     */
    int debug;
    int clean_tmp;

    // experimental
    int css_draw;
};

} // namespace pdf2htmlEX

#endif //PARAM_h__
