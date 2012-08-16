/*
 * export.cc
 *
 * Export styles to HTML
 *
 * by WangLu
 * 2012.08.14
 */

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "HTMLRenderer.h"
#include "namespace.h"

using boost::algorithm::ifind_first;

void HTMLRenderer::export_remote_font(long long fn_id, const string & suffix, const string & fontfileformat, GfxFont * font)
{
    allcss_fout << format("@font-face{font-family:f%|1$x|;src:url(") % fn_id;

    const std::string fn = (format("f%|1$x|%2%") % fn_id % suffix).str();
    if(param->single_html)
    {
        allcss_fout << "'data:font/" << fontfileformat << ";base64," << base64stream(ifstream(tmp_dir / fn, ifstream::binary)) << "'";
    }
    else
    {
        allcss_fout << fn;
    }

    allcss_fout << format(")format(\"%1%\");}.f%|2$x|{font-family:f%|2$x|;") % fontfileformat % fn_id;

    double a = font->getAscent();
    double d = font->getDescent();
    double r = _is_positive(a-d) ? (a/(a-d)) : 1.0;

    for(const string & prefix : {"", "-ms-", "-moz-", "-webkit-", "-o-"})
    {
        allcss_fout << prefix << "transform-origin:0% " << (r*100.0) << "%;";
    }

    allcss_fout << "line-height:" << (a-d) << ";";

    allcss_fout << "}" << endl;
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
    allcss_fout << format(".f%|1$x|{font-family:sans-serif;color:transparent;visibility:hidden;}")%fn_id << endl;
}

void HTMLRenderer::export_local_font(long long fn_id, GfxFont * font, const string & original_font_name, const string & cssfont)
{
    allcss_fout << format(".f%|1$x|{") % fn_id;
    allcss_fout << "font-family:" << ((cssfont == "") ? (original_font_name + "," + general_font_family(font)) : cssfont) << ";";

    if(font->isBold())
        allcss_fout << "font-weight:bold;";

    if(ifind_first(original_font_name, "oblique"))
        allcss_fout << "font-style:oblique;";
    else if(font->isItalic())
        allcss_fout << "font-style:italic;";

    double a = font->getAscent();
    double d = font->getDescent();
    double r = _is_positive(a-d) ? (a/(a-d)) : 1.0;

    for(const string & prefix : {"", "-ms-", "-moz-", "-webkit-", "-o-"})
    {
        allcss_fout << prefix << "transform-origin:0% " << (r*100.0) << "%;";
    }

    allcss_fout << "line-height:" << (a-d) << ";";

    allcss_fout << "}" << endl;
}


void HTMLRenderer::export_font_size (long long fs_id, double font_size)
{
    allcss_fout << format(".s%|1$x|{font-size:%2%px;}") % fs_id % font_size << endl;
}

void HTMLRenderer::export_transform_matrix (long long tm_id, const double * tm)
{
    allcss_fout << format(".t%|1$x|{") % tm_id;


    // TODO: recognize common matices
    if(_tm_equal(tm, id_matrix))
    {
        // no need to output anything
    }
    else
    {
        for(const string & prefix : {"", "-ms-", "-moz-", "-webkit-", "-o-"})
        {
            // PDF use a different coordinate system from Web
            allcss_fout << prefix << "transform:matrix("
                << tm[0] << ','
                << -tm[1] << ','
                << -tm[2] << ','
                << tm[3] << ',';

            // we have already shifted the origin
            allcss_fout << "0,0);";
            /*
            if(prefix == "-moz-")
                allcss_fout << format("%1%px,%2%px);") % tm[4] % -tm[5];
            else
                allcss_fout << format("%1%,%2%);") % tm[4] % -tm[5];
                */
        }
    }
    allcss_fout << "}" << endl;
}

void HTMLRenderer::export_letter_space (long long ls_id, double letter_space)
{
    allcss_fout << format(".l%|1$x|{letter-spacing:%2%px;}") % ls_id % letter_space << endl;
}

void HTMLRenderer::export_word_space (long long ws_id, double word_space)
{
    allcss_fout << format(".w%|1$x|{word-spacing:%2%px;}") % ws_id % word_space << endl;
}

void HTMLRenderer::export_color (long long color_id, const GfxRGB * rgb)
{
    allcss_fout << format(".c%|1$x|{color:rgb(%2%,%3%,%4%);}") 
        % color_id % (int)colToByte(rgb->r) % (int)colToByte(rgb->g) % (int)colToByte(rgb->b)
        << endl;
}

void HTMLRenderer::export_whitespace (long long ws_id, double ws_width)
{
    allcss_fout << format("._%|1$x|{width:%2%px;}") % ws_id % ws_width << endl;
}

