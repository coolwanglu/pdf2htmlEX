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

void HTMLRenderer::export_remote_font(const FontInfo & info, const string & suffix, const string & fontfileformat, GfxFont * font) 
{
    allcss_fout << "@font-face{font-family:f" << info.id << ";src:url(";

    {
        auto fn = str_fmt("f%llx%s", info.id, suffix.c_str());
        if(param->single_html)
        {
            allcss_fout << "'data:font/" << fontfileformat << ";base64," << base64stream(ifstream(tmp_dir + "/" + (char*)fn, ifstream::binary)) << "'";
        }
        else
        {
            allcss_fout << (char*)fn;
        }
    }

    allcss_fout << ")format(\"" << fontfileformat << "\");}.f" << info.id << "{font-family:f" << info.id << ";line-height:" << (info.ascent - info.descent) << ";}" << endl;
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
    allcss_fout << ".f" << fn_id << "{font-family:sans-serif;color:transparent;visibility:hidden;}" << endl;
}

void HTMLRenderer::export_local_font(const FontInfo & info, GfxFont * font, const string & original_font_name, const string & cssfont) 
{
    allcss_fout << ".f" << info.id << "{";
    allcss_fout << "font-family:" << ((cssfont == "") ? (original_font_name + "," + general_font_family(font)) : cssfont) << ";";

    string fn = original_font_name;
    for(auto iter = fn.begin(); iter != fn.end(); ++iter)
        *iter = tolower(*iter);

    if(font->isBold() || (fn.find("bold") != string::npos))
        allcss_fout << "font-weight:bold;";

    if(fn.find("oblique") != string::npos)
        allcss_fout << "font-style:oblique;";
    else if(font->isItalic() || (fn.find("italic") != string::npos))
        allcss_fout << "font-style:italic;";

    allcss_fout << "line-height:" << (info.ascent - info.descent) << ";";

    allcss_fout << "}" << endl;
}

void HTMLRenderer::export_font_size (long long fs_id, double font_size) 
{
    allcss_fout << ".s" << fs_id << "{font-size:" << font_size << "px;}" << endl;
}

void HTMLRenderer::export_transform_matrix (long long tm_id, const double * tm) 
{
    allcss_fout << ".t" << tm_id << "{";

    // always ignore tm[4] and tm[5] because
    // we have already shifted the origin
    
    // TODO: recognize common matices
    auto prefixes = {"", "-ms-", "-moz-", "-webkit-", "-o-"};
    for(auto iter = prefixes.begin(); iter != prefixes.end(); ++iter)
    {
        const auto & prefix = *iter;
        // PDF use a different coordinate system from Web
        allcss_fout << prefix << "transform:matrix("
            << tm[0] << ','
            << -tm[1] << ','
            << -tm[2] << ','
            << tm[3] << ',';

        allcss_fout << "0,0);";
    }
    allcss_fout << "}" << endl;
}

void HTMLRenderer::export_letter_space (long long ls_id, double letter_space) 
{
    allcss_fout << ".l" << ls_id << "{letter-spacing:" << letter_space << "px;}" << endl;
}

void HTMLRenderer::export_word_space (long long ws_id, double word_space) 
{
    allcss_fout << ".w" << ws_id << "{word-spacing:" << word_space << "px;}" << endl;
}

void HTMLRenderer::export_color (long long color_id, const GfxRGB * rgb) 
{
    allcss_fout << ".c" << color_id << "{color:rgb("
        << dec << (int)colToByte(rgb->r) << "," << (int)colToByte(rgb->g) << "," << (int)colToByte(rgb->b) << ");}" << hex
        << endl;
}

void HTMLRenderer::export_whitespace (long long ws_id, double ws_width) 
{
    if(ws_width > 0)
        allcss_fout << "._" << ws_id << "{display:inline-block;width:" << ws_width << "px;}" << endl;
    else
        allcss_fout << "._" << ws_id << "{display:inline;margin-left:" << ws_width << "px;}" << endl;
}

void HTMLRenderer::export_rise (long long rise_id, double rise) 
{
    allcss_fout << ".r" << rise_id << "{top:" << (-rise) << "px;}" << endl;
}

