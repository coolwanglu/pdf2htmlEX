// pdftohtmlEX.cc
//
// Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <string>
#include <limits>
#include <iostream>
#include <getopt.h>

#include <goo/GooString.h>

#include <Object.h>
#include <PDFDoc.h>
#include <PDFDocFactory.h>
#include <GlobalParams.h>

#include "Param.h"
#include "pdf2htmlEX-config.h"
#include "HTMLRenderer/HTMLRenderer.h"
#include "util/ArgParser.h"
#include "util/path.h"

using namespace std;
using namespace pdf2htmlEX;

Param param;
ArgParser argparser;

void show_usage_and_exit(const char * dummy = nullptr)
{
    cerr << "pdftohtmlEX version " << PDF2HTMLEX_VERSION << endl;
    cerr << endl;
    cerr << "Copyright 2012 Lu Wang <coolwanglu@gmail.com>" << endl;
    cerr << endl;
    cerr << "Usage: pdf2htmlEX [Options] <input.pdf> [<output.html>]" << endl;
    cerr << endl;
    cerr << "Options:" << endl;
    argparser.show_usage(cerr);
    cerr << endl;
    cerr << "Run 'man pdf2htmlEX' for detailed information" << endl;
    cerr << endl;
    exit(EXIT_FAILURE);
}

void parse_options (int argc, char **argv)
{
    argparser
        .add("help,h", "show all options", &show_usage_and_exit)
        .add("version,v", "show copyright and version info", &show_usage_and_exit)

        .add("owner-password,o", &param.owner_password, "", "owner password (for encrypted files)", nullptr, true)
        .add("user-password,u", &param.user_password, "", "user password (for encrypted files)", nullptr, true)

        .add("dest-dir", &param.dest_dir, ".", "specify destination directory")
        .add("data-dir", &param.data_dir, PDF2HTMLEX_DATA_PATH, "specify data directory")

        .add("first-page,f", &param.first_page, 1, "first page to process")
        .add("last-page,l", &param.last_page, numeric_limits<int>::max(), "last page to process")

        .add("zoom", &param.zoom, 0, "zoom ratio", nullptr, true)
        .add("fit-width", &param.fit_width, 0, "fit width", nullptr, true) 
        .add("fit-height", &param.fit_height, 0, "fit height", nullptr, true)
	.add("fit-every", &param.fit_every, 0, "fit every page to fit-width/height", nullptr, true)
        .add("hdpi", &param.h_dpi, 144.0, "horizontal DPI for non-text")
        .add("vdpi", &param.v_dpi, 144.0, "vertical DPI for non-text")
        .add("use-cropbox", &param.use_cropbox, 0, "use CropBox instead of MediaBox")

        .add("process-nontext", &param.process_nontext, 1, "process nontext objects")
        .add("single-html", &param.single_html, 1, "combine everything into one single HTML file")
        .add("split-pages", &param.split_pages, 0, "split pages into separated files")
        .add("embed-base-font", &param.embed_base_font, 0, "embed local matched font for base 14 fonts in the PDF file")
        .add("embed-external-font", &param.embed_external_font, 0, "embed local matched font for external fonts in the PDF file")
        .add("decompose-ligature", &param.decompose_ligature, 0, "decompose ligatures, for example 'fi' -> 'f''i'")

        .add("heps", &param.h_eps, 1.0, "max tolerated horizontal offset (in pixels)")
        .add("veps", &param.v_eps, 1.0, "max tolerated vertical offset (in pixels)")
        .add("space-threshold", &param.space_threshold, (1.0/8), "distance no thiner than (threshold * em) will be considered as a space character")
        .add("font-size-multiplier", &param.font_size_multiplier, 4.0, "setting a value greater than 1 would increase the rendering accuracy")
        .add("auto-hint", &param.auto_hint, 0, "Whether to generate hints for fonts")
        .add("tounicode", &param.tounicode, 0, "Specify how to deal with ToUnicode map, 0 for auto, 1 for forced, -1 for disabled")
        .add("space-as-offset", &param.space_as_offset, 0, "treat space characters as offsets")
        .add("stretch-narrow-glyph", &param.stretch_narrow_glyph, 0, "stretch narrow glyphs instead of padding space")
        .add("squeeze-wide-glyph", &param.squeeze_wide_glyph, 1, "squeeze wide glyphs instead of truncating")
        .add("remove-unused-glyph", &param.remove_unused_glyph, 1, "remove unused glyphs in embedded fonts")

        .add("font-suffix", &param.font_suffix, ".ttf", "suffix for extracted font files")
        .add("font-format", &param.font_format, "opentype", "format for extracted font files")
        .add("external-hint-tool", &param.external_hint_tool, "", "external tool for hintting fonts.(overrides --auto-hint)")
        .add("css-filename", &param.css_filename, "", "Specify the file name of the generated css file")

        .add("debug", &param.debug, 0, "output debug information")
        .add("clean-tmp", &param.clean_tmp, 1, "clean temporary files after processing")
        .add("css-draw", &param.css_draw, 0, "[Experimental and Unsupported] CSS Drawing")
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
        cerr << "Missing input filename" << endl;
        exit(EXIT_FAILURE);
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
    PDFDoc *doc = nullptr;
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
            throw "Copying of text from this document is not allowed.";
        }

        param.first_page = min<int>(max<int>(param.first_page, 1), doc->getNumPages());
        param.last_page = min<int>(max<int>(param.last_page, param.first_page), doc->getNumPages());

        if(param.output_filename == "")
        {
            const string s = get_filename(param.input_filename);

            if(get_suffix(param.input_filename) == ".pdf")
            {
                if(param.split_pages)
                    param.output_filename = s.substr(0, s.size() - 4);
                else
                    param.output_filename = s.substr(0, s.size() - 4) + ".html";

            }
            else
            {
                if(param.split_pages)
                    param.output_filename = s;
                else
                    param.output_filename = s + ".html";
                
            }
        }
        if(param.css_filename == "")
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

        HTMLRenderer * htmlOut = new HTMLRenderer(&param);
        htmlOut->process(doc);
        delete htmlOut;

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
    if(doc) delete doc;
    if(globalParams) delete globalParams;

    // check for memory leaks
    Object::memCheck(stderr);
    gMemReport(stderr);

    exit(finished ? (EXIT_SUCCESS) : (EXIT_FAILURE));

    return 0;
}
