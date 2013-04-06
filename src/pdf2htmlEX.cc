// pdftohtmlEX.cc
//
// Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <string>
#include <limits>
#include <iostream>
#include <memory>

#include <getopt.h>

#include <poppler-config.h>
#include <goo/GooString.h>

#include <Object.h>
#include <PDFDoc.h>
#include <PDFDocFactory.h>
#include <GlobalParams.h>

#include "pdf2htmlEX-config.h"
#include "ArgParser.h"
#include "Param.h"
#include "HTMLRenderer/HTMLRenderer.h"

#include "util/path.h"
#include "util/ffw.h"

using namespace std;
using namespace pdf2htmlEX;

Param param;
ArgParser argparser;

void show_usage_and_exit(const char * dummy = nullptr)
{
    cerr << "Usage: pdf2htmlEX [options] <input.pdf> [<output.html>]" << endl;
    argparser.show_usage(cerr);
    exit(EXIT_FAILURE);
}

void show_version_and_exit(const char * dummy = nullptr)
{
    cerr << "pdftohtmlEX version " << PDF2HTMLEX_VERSION << endl;
    cerr << "Copyright 2012,2013 Lu Wang <coolwanglu@gmail.com>" << endl;
    cerr << "Libraries: ";
    cerr << "poppler " << POPPLER_VERSION << ", ";
    cerr << "libfontforge " << ffw_get_version() << endl;
    cerr << "Default data-dir: " << PDF2HTMLEX_DATA_PATH << endl;
    exit(EXIT_SUCCESS);
}

void parse_options (int argc, char **argv)
{
    argparser
        // pages
        .add("first-page,f", &param.first_page, 1, "first page to convert")
        .add("last-page,l", &param.last_page, numeric_limits<int>::max(), "last page to convert")
        
        // dimensions
        .add("zoom", &param.zoom, 0, "zoom ratio", nullptr, true)
        .add("fit-width", &param.fit_width, 0, "fit width to <fp> pixels", nullptr, true) 
        .add("fit-height", &param.fit_height, 0, "fit height to <fp> pixels", nullptr, true)
        .add("use-cropbox", &param.use_cropbox, 1, "use CropBox instead of MediaBox")
        .add("hdpi", &param.h_dpi, 144.0, "horizontal resolution for graphics in DPI")
        .add("vdpi", &param.v_dpi, 144.0, "vertical resolution for graphics in DPI")
        
        // output files
        .add("single-html", &param.single_html, 1, "generate a single HTML file")
        .add("split-pages", &param.split_pages, 0, "split pages into separate files")
        .add("dest-dir", &param.dest_dir, ".", "specify destination directory")
        .add("css-filename", &param.css_filename, "", "filename of the generated css file")
        .add("outline-filename", &param.outline_filename, "", "filename of the generated outline file")
        .add("process-nontext", &param.process_nontext, 1, "render graphics in addition to text")
        .add("process-outline", &param.process_outline, 1, "show outline in HTML")
        .add("fallback", &param.fallback, 0, "output in fallback mode")
        
        // fonts
        .add("embed-base-font", &param.embed_base_font, 1, "embed local match for standard 14 fonts")
        .add("embed-external-font", &param.embed_external_font, 0, "embed local match for external fonts")
        .add("font-suffix", &param.font_suffix, ".ttf", "suffix for embedded font files (.ttf,.otf,.woff,.svg)")
        .add("decompose-ligature", &param.decompose_ligature, 0, "decompose ligatures, such as \uFB01 -> fi")
        .add("remove-unused-glyph", &param.remove_unused_glyph, 1, "remove unused glyphs in embedded fonts")
        .add("auto-hint", &param.auto_hint, 0, "use fontforge autohint on fonts without hints")
        .add("external-hint-tool", &param.external_hint_tool, "", "external tool for hinting fonts (overrides --auto-hint)")
        .add("stretch-narrow-glyph", &param.stretch_narrow_glyph, 0, "stretch narrow glyphs instead of padding them")
        .add("squeeze-wide-glyph", &param.squeeze_wide_glyph, 1, "shrink wide glyphs instead of truncating them")
        
        // text
        .add("heps", &param.h_eps, 1.0, "horizontal threshold for merging text, in pixels")
        .add("veps", &param.v_eps, 1.0, "vertical threshold for merging text, in pixels")
        .add("space-threshold", &param.space_threshold, (1.0/8), "word break threshold (threshold * em)")
        .add("font-size-multiplier", &param.font_size_multiplier, 4.0, "a value greater than 1 increases the rendering accuracy")
        .add("space-as-offset", &param.space_as_offset, 0, "treat space characters as offsets")
        .add("tounicode", &param.tounicode, 0, "how to handle ToUnicode CMaps (0=auto, 1=force, -1=ignore)")
        .add("optimize-text", &param.optimize_text, 1, "try to reduce the number of HTML elements used for text")
        
        // encryption
        .add("owner-password,o", &param.owner_password, "", "owner password (for encrypted files)", nullptr, true)
        .add("user-password,u", &param.user_password, "", "user password (for encrypted files)", nullptr, true)
        .add("no-drm", &param.no_drm, 0, "override document DRM settings")
        
        // misc.
        .add("clean-tmp", &param.clean_tmp, 1, "remove temporary files after conversion")
        .add("data-dir", &param.data_dir, PDF2HTMLEX_DATA_PATH, "specify data directory")
        // TODO: css drawings are hidden on print, for annot links, need to fix it for other drawings
//        .add("css-draw", &param.css_draw, 0, "[experimental and unsupported] CSS drawing")
        .add("debug", &param.debug, 0, "print debugging information")
        
        // meta
        .add("version,v", "print copyright and version info", &show_version_and_exit)
        .add("help,h", "print usage information", &show_usage_and_exit)
        
        .add("", &param.input_filename, "", "")
        .add("", &param.output_filename, "", "")
        ;

    try
    {
        argparser.parse(argc, argv);
    }
    catch(const char * s)
    {
        // if s == "", getopt_long would have printed the error message
        if(s && s[0])
        {
            cerr << "Error when parsing the arguments:" << endl;
            cerr << s << endl;
        }
        exit(EXIT_FAILURE);
    }
    catch(const std::string & s)
    {
        // if s == "", getopt_long would have printed the error message
        if(s != "")
        {
            cerr << "Error when parsing the arguments:" << endl;
            cerr << s << endl;
        }
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    parse_options(argc, argv);
    if (param.input_filename == "")
    {
        show_usage_and_exit();
    }

    //prepare the directories
    {
        char buf[] = "/tmp/pdf2htmlEX-XXXXXX";
        auto p = mkdtemp(buf);
        if(p == nullptr)
        {
            cerr << "Cannot create temp directory" << endl;
            exit(EXIT_FAILURE);
        }
        param.tmp_dir = buf;
    }

    if(param.debug)
        cerr << "temporary dir: " << (param.tmp_dir) << endl;

    try
    {
        create_directories(param.dest_dir);
    }
    catch (const string & s)
    {
        cerr << s << endl;
        exit(EXIT_FAILURE);
    }

    bool finished = false;
    // read config file
    globalParams = new GlobalParams();
    // open PDF file
    PDFDoc * doc = nullptr;
    try
    {
        {
            GooString * ownerPW = (param.owner_password == "") ? (nullptr) : (new GooString(param.owner_password.c_str()));
            GooString * userPW = (param.user_password == "") ? (nullptr) : (new GooString(param.user_password.c_str()));
            GooString fileName(param.input_filename.c_str());

            doc = PDFDocFactory().createPDFDoc(fileName, ownerPW, userPW);

            delete userPW;
            delete ownerPW;
        }

        if (!doc->isOk()) {
            throw "Cannot read the file";
        }

        // check for copy permission
        if (!doc->okToCopy()) {
            if (param.no_drm == 0) {
                throw "Copying of text from this document is not allowed.";
            }
            cerr << "Document has copy-protection bit set." << endl;
        }

        param.first_page = min<int>(max<int>(param.first_page, 1), doc->getNumPages());
        param.last_page = min<int>(max<int>(param.last_page, param.first_page), doc->getNumPages());

        if(param.output_filename.empty())
        {
            const string s = get_filename(param.input_filename);

            if(get_suffix(param.input_filename) == ".pdf")
            {
                if(param.split_pages)
                {
                    param.output_filename = s.substr(0, s.size() - 4) + "%d.page";
                    sanitize_filename(param.output_filename);
                }
                else
                {
                    param.output_filename = s.substr(0, s.size() - 4) + ".html";
                }

            }
            else
            {
                if(param.split_pages)
                {
                    param.output_filename = s + "%d.page";
                    sanitize_filename(param.output_filename);
                }
                else
                {
                    param.output_filename = s + ".html";
                }
                
            }
        }
		else if(param.split_pages)
        {
            // Need to make sure we have a page number placeholder in the filename
            if(!sanitize_filename(param.output_filename))
            {
                // Inject the placeholder just before the file extension
                const string suffix = get_suffix(param.output_filename);
                param.output_filename = param.output_filename.substr(0, param.output_filename.size() - suffix.size()) + "%d" + suffix;
                sanitize_filename(param.output_filename);
            }
        }
        if(param.css_filename.empty())
        {
            const string s = get_filename(param.input_filename);

            if(get_suffix(param.input_filename) == ".pdf")
            {
                param.css_filename = s.substr(0, s.size() - 4) + ".css";
            }
            else
            {
                if(!param.split_pages)
                    param.css_filename = s + ".css";
            }
        }
        if(param.outline_filename.empty())
        {
            const string s = get_filename(param.input_filename);

            if(get_suffix(param.input_filename) == ".pdf")
            {
                param.outline_filename = s.substr(0, s.size() - 4) + ".outline";
            }
            else
            {
                if(!param.split_pages)
                    param.outline_filename = s + ".outline";
            }

        }

        unique_ptr<HTMLRenderer>(new HTMLRenderer(param))->process(doc);

        finished = true;
    }
    catch (const char * s)
    {
        cerr << "Error: " << s << endl;
    }
    catch (const string & s)
    {
        cerr << "Error: " << s << endl;
    }

    // clean up
    delete doc;
    delete globalParams;

    // check for memory leaks
    Object::memCheck(stderr);
    gMemReport(stderr);

    exit(finished ? (EXIT_SUCCESS) : (EXIT_FAILURE));

    return 0;
}
