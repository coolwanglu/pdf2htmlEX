/*
 * export.cc
 *
 * Export styles to HTML
 *
 * by WangLu
 * 2012.08.14
 */

#include <sstream>
#include <cctype>

#include "HTMLRenderer.h"
#include "util/namespace.h"
#include "util/base64stream.h"
#include "util/math.h"
#include "util/misc.h"

namespace pdf2htmlEX {

using std::cerr;
using std::endl;

void HTMLRenderer::export_remote_font(const FontInfo & info, const string & suffix, GfxFont * font)
{
    string mime_type, format;
    if(suffix == ".ttf")
    {
        format = "truetype";
        mime_type = "application/x-font-ttf";
    }
    else if(suffix == ".otf")
    {
        format = "opentype";
        mime_type = "application/x-font-otf";
    }
    else if(suffix == ".woff")
    {
        format = "woff";
        mime_type = "application/font-woff";
    }
    else if(suffix == ".eot")
    {
        format = "embedded-opentype";
        mime_type = "application/vnd.ms-fontobject";
    }
    else if(suffix == ".svg")
    {
        format = "svg";
        mime_type = "image/svg+xml";
    }
    else
    {
        cerr << "Warning: unknown font suffix: " << suffix << endl;
    }

    f_css.fs << "@font-face{"
             << "font-family:f" << info.id << ";"
             << "src:url(";

    {
        auto fn = str_fmt("f%llx%s", info.id, suffix.c_str());
        if(param->single_html)
        {
            auto path = param->tmp_dir + "/" + (char*)fn;
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw "Cannot locate font file: " + path;
            f_css.fs << "'data:font/" + mime_type + ";base64," << base64stream(fin) << "'";
        }
        else
        {
            f_css.fs << (char*)fn;
        }
    }

    f_css.fs << ")"
             << "format(\"" << format << "\");"
             << "}" // end of @font-face
             << ".f" << info.id << "{"
             << "font-family:f" << info.id << ";"
             << "line-height:" << round(info.ascent - info.descent) << ";"
             << "font-style:normal;"
             << "font-weight:normal;"
             << "visibility:visible;"
             << "}" // end of .f
             << endl;
}

static string general_font_family(GfxFont * font)
{
    if(font->isFixedWidth())
        return "monospace";
    else if (font->isSerif())
        return "serif";
    else
        return "sans-serif";
}

// TODO: this function is called when some font is unable to process, may use the name there as a hint
void HTMLRenderer::export_remote_default_font(long long fn_id) 
{
    f_css.fs << ".f" << fn_id << "{font-family:sans-serif;visibility:hidden;}" << endl;
}

void HTMLRenderer::export_local_font(const FontInfo & info, GfxFont * font, const string & original_font_name, const string & cssfont) 
{
    f_css.fs << ".f" << info.id << "{";
    f_css.fs << "font-family:" << ((cssfont == "") ? (original_font_name + "," + general_font_family(font)) : cssfont) << ";";

    string fn = original_font_name;
    for(auto iter = fn.begin(); iter != fn.end(); ++iter)
        *iter = tolower(*iter);

    if(font->isBold() || (fn.find("bold") != string::npos))
        f_css.fs << "font-weight:bold;";
    else
        f_css.fs << "font-weight:normal;";

    if(fn.find("oblique") != string::npos)
        f_css.fs << "font-style:oblique;";
    else if(font->isItalic() || (fn.find("italic") != string::npos))
        f_css.fs << "font-style:italic;";
    else
        f_css.fs << "font-style:normal;";

    f_css.fs << "line-height:" << round(info.ascent - info.descent) << ";";

    f_css.fs << "visibility:visible;";

    f_css.fs << "}" << endl;
}

void HTMLRenderer::export_font_size (long long fs_id, double font_size) 
{
    f_css.fs << ".s" << fs_id << "{font-size:" << round(font_size) << "px;}" << endl;
}

void HTMLRenderer::export_transform_matrix (long long tm_id, const double * tm) 
{
    f_css.fs << ".t" << tm_id << "{";

    // always ignore tm[4] and tm[5] because
    // we have already shifted the origin
    
    // TODO: recognize common matices
    if(tm_equal(tm, ID_MATRIX, 4))
    {
        auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
        for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
            f_css.fs << *iter << "transform:none;";
    }
    else
    {
        auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
        for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
        {
            // PDF use a different coordinate system from Web
            f_css.fs << *iter << "transform:matrix("
                << round(tm[0]) << ','
                << round(-tm[1]) << ','
                << round(-tm[2]) << ','
                << round(tm[3]) << ',';

            f_css.fs << "0,0);";
        }
    }
    f_css.fs << "}" << endl;
}

void HTMLRenderer::export_word_space (long long ws_id, double word_space) 
{
    f_css.fs << ".w" << ws_id << "{word-spacing:" << round(word_space) << "px;}" << endl;
}

void HTMLRenderer::export_fill_color (long long color_id, const GfxRGB * rgb) 
{
    f_css.fs << ".c" << color_id << "{color:" << *rgb << ";}" << endl;
}

void HTMLRenderer::export_stroke_color (long long color_id, const GfxRGB * rgb) 
{   
    // TODO: take the stroke width from the graphics state,
    //       currently using 0.015em as a good default
    
    // Firefox, IE, etc.
    f_css.fs << ".C" << color_id << "{"
             << "text-shadow: "
             << "-0.015em 0 "  << *rgb << "," 
             << "0 0.015em "   << *rgb << ","
             << "0.015em 0 "   << *rgb << ","
             << "0 -0.015em  " << *rgb << ";"
             << "}" << endl;
    
    // WebKit
    f_css.fs << "@media screen and (-webkit-min-device-pixel-ratio:0) {";
    f_css.fs << ".C" << color_id << "{-webkit-text-stroke: 0.015em " << *rgb << ";text-shadow: none;}";
    f_css.fs << "}";
}

void HTMLRenderer::export_whitespace (long long ws_id, double ws_width) 
{
    if(ws_width > 0)
        f_css.fs << "._" << ws_id << "{display:inline-block;width:" << round(ws_width) << "px;}" << endl;
    else
        f_css.fs << "._" << ws_id << "{display:inline;margin-left:" << round(ws_width) << "px;}" << endl;
}

void HTMLRenderer::export_rise (long long rise_id, double rise) 
{
    f_css.fs << ".r" << rise_id << "{top:" << round(-rise) << "px;}" << endl;
}

void HTMLRenderer::export_height (long long height_id, double height) 
{
    f_css.fs << ".h" << height_id << "{height:" << round(height) << "px;}" << endl;
}
void HTMLRenderer::export_left (long long left_id, double left) 
{
    f_css.fs << ".L" << left_id << "{left:" << round(left) << "px;}" << endl;
}

}
