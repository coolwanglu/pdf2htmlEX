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
#include "namespace.h"

namespace pdf2htmlEX {

void HTMLRenderer::export_remote_font(const FontInfo & info, const string & suffix, const string & fontfileformat, GfxFont * font) 
{
    css_fout << "@font-face{font-family:f" << info.id << ";src:url(";

    {
        auto fn = str_fmt("f%llx%s", info.id, suffix.c_str());
        if(param->single_html)
        {
            auto path = param->tmp_dir + "/" + (char*)fn;
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw "Cannot locate font file: " + path;
            css_fout << "'data:font/" + fontfileformat + ";base64," << base64stream(fin) << "'";
        }
        else
        {
            css_fout << (char*)fn;
        }
    }

    css_fout << ")format(\"" << fontfileformat 
        << "\");}.f" << info.id 
        << "{font-family:f" << info.id 
        << ";line-height:" << _round(info.ascent - info.descent) 
        << ";font-style:normal;font-weight:normal;}";

    css_fout << endl;
}

static string general_font_family(GfxFont * font)
{
    if(font -> isFixedWidth())
        return "monospace";
    else if (font -> isSerif())
        return "serif";
    else
        return "sans-serif";
}

// TODO: this function is called when some font is unable to process, may use the name there as a hint
void HTMLRenderer::export_remote_default_font(long long fn_id) 
{
    css_fout << ".f" << fn_id << "{font-family:sans-serif;color:transparent;visibility:hidden;}" << endl;
}

void HTMLRenderer::export_local_font(const FontInfo & info, GfxFont * font, const string & original_font_name, const string & cssfont) 
{
    css_fout << ".f" << info.id << "{";
    css_fout << "font-family:" << ((cssfont == "") ? (original_font_name + "," + general_font_family(font)) : cssfont) << ";";

    string fn = original_font_name;
    for(auto iter = fn.begin(); iter != fn.end(); ++iter)
        *iter = tolower(*iter);

    if(font->isBold() || (fn.find("bold") != string::npos))
        css_fout << "font-weight:bold;";
    else
        css_fout << "font-weight:normal;";

    if(fn.find("oblique") != string::npos)
        css_fout << "font-style:oblique;";
    else if(font->isItalic() || (fn.find("italic") != string::npos))
        css_fout << "font-style:italic;";
    else
        css_fout << "font-style:normal;";

    css_fout << "line-height:" << _round(info.ascent - info.descent) << ";";

    css_fout << "}" << endl;
}

void HTMLRenderer::export_font_size (long long fs_id, double font_size) 
{
    css_fout << ".s" << fs_id << "{font-size:" << _round(font_size) << "px;}" << endl;
}

void HTMLRenderer::export_transform_matrix (long long tm_id, const double * tm) 
{
    css_fout << ".t" << tm_id << "{";

    // always ignore tm[4] and tm[5] because
    // we have already shifted the origin
    
    // TODO: recognize common matices
    if(_tm_equal(tm, id_matrix, 4))
    {
        auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
        for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
            css_fout << *iter << "transform:none;";
    }
    else
    {
        auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
        for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
        {
            // PDF use a different coordinate system from Web
            css_fout << *iter << "transform:matrix("
                << _round(tm[0]) << ','
                << _round(tm[1]) << ','
                << _round(-tm[2]) << ','
                << _round(-tm[3]) << ',';

            css_fout << "0,0);";
        }
    }
    css_fout << "}" << endl;
}

void HTMLRenderer::export_letter_space (long long ls_id, double letter_space) 
{
    css_fout << ".l" << ls_id << "{letter-spacing:" << _round(letter_space) << "px;}" << endl;
}

void HTMLRenderer::export_word_space (long long ws_id, double word_space) 
{
    css_fout << ".w" << ws_id << "{word-spacing:" << _round(word_space) << "px;}" << endl;
}

void HTMLRenderer::export_color (long long color_id, const GfxRGB * rgb) 
{
    css_fout << ".c" << color_id << "{color:" << (*rgb) << ";}" << endl;
}

void HTMLRenderer::export_whitespace (long long ws_id, double ws_width) 
{
    if(ws_width > 0)
        css_fout << "._" << ws_id << "{display:inline-block;width:" << _round(ws_width) << "px;}" << endl;
    else
        css_fout << "._" << ws_id << "{display:inline;margin-left:" << _round(ws_width) << "px;}" << endl;
}

void HTMLRenderer::export_rise (long long rise_id, double rise) 
{
    css_fout << ".r" << rise_id << "{top:" << _round(-rise) << "px;}" << endl;
}

void HTMLRenderer::export_height (long long height_id, double height) 
{
    css_fout << ".h" << height_id << "{height:" << _round(height) << "px;}" << endl;
}

}
