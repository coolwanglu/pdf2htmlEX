// pdftohtmlEX.cc
//
// Copyright (C) 2012 Lu Wang coolwanglu<at>gmail.com

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <string>
#include <limits>
#include <iostream>

#include <boost/program_options.hpp>

#include <goo/GooString.h>

#include <Object.h>
#include <PDFDoc.h>
#include <PDFDocFactory.h>
#include <GlobalParams.h>

#include "HTMLRenderer.h"
#include "Param.h"
#include "config.h"

namespace po = boost::program_options;
using namespace std;

Param param;

// variables
PDFDoc *doc = nullptr;
GooString *fileName = nullptr;
GooString *ownerPW, *userPW;

HTMLRenderer *htmlOut = nullptr;

bool finished = false;

po::options_description opt_visible("Options"), opt_hidden, opt_all;
po::positional_options_description opt_positional;

void show_usage(void)
{
    cerr << "pdftohtmlEX version " << PDF2HTMLEX_VERSION << endl;
    cerr << endl;
    cerr << "Copyright 2012 Lu Wang (coolwanglu<at>gmail.com)" << endl;
    cerr << endl;
    cerr << "Usage: pdf2htmlEX [Options] <PDF-file>" << endl;
    cerr << endl;
    cerr << opt_visible << endl;
}

po::variables_map parse_options (int argc, char **argv)
{
    opt_visible.add_options()
        ("help", "show all options")
        ("version,v", "show copyright and version info")

        ("owner-password,o", po::value<string>(&param.owner_password)->default_value(""), "owner password (for encrypted files)")
        ("user-password,u", po::value<string>(&param.user_password)->default_value(""), "user password (for encrypted files)")

        ("dest-dir", po::value<string>(&param.dest_dir)->default_value("."), "destination directory")

        ("first-page,f", po::value<int>(&param.first_page)->default_value(1), "first page to process")
        ("last-page,l", po::value<int>(&param.last_page)->default_value(numeric_limits<int>::max()), "last page to process")

        ("zoom", po::value<double>(&param.zoom)->default_value(1.0), "zoom ratio")
        ("hdpi", po::value<double>(&param.h_dpi)->default_value(144.0), "horizontal DPI for non-text")
        ("vdpi", po::value<double>(&param.v_dpi)->default_value(144.0), "vertical DPI for non-text")

        ("process-nontext", po::value<int>(&param.process_nontext)->default_value(1), "process nontext objects")
        ("single-html", po::value<int>(&param.single_html)->default_value(1), "combine everything into one single HTML file")
        ("embed-base-font", po::value<int>(&param.embed_base_font)->default_value(0), "embed local matched font for base 14 fonts in the PDF file")
        ("embed-external-font", po::value<int>(&param.embed_external_font)->default_value(0), "embed local matched font for external fonts in the PDF file")
        ("decompose-ligature", po::value<int>(&param.decompose_ligature)->default_value(0), "decompose ligatures, for example 'fi' -> 'f''i'")

        ("heps", po::value<double>(&param.h_eps)->default_value(1.0), "max tolerated horizontal offset (in pixels)")
        ("veps", po::value<double>(&param.v_eps)->default_value(1.0), "max tolerated vertical offset (in pixels)")
        ("space-threshold", po::value<double>(&param.space_threshold)->default_value(1.0/8), "distance no thiner than (threshold * em) will be considered as a space character")
        ("font-size-multiplier", po::value<double>(&param.font_size_multiplier)->default_value(10.0), "setting a value greater than 1 would increase the rendering accuracy")
        ("tounicode", po::value<int>(&param.tounicode)->default_value(0), "Specify how to deal with ToUnicode map, 0 for auto, 1 for forced, -1 for disabled")
        ("space-as-offset", po::value<int>(&param.space_as_offset)->default_value(0), "treat space characters as offsets")

        ("font-suffix", po::value<string>(&param.font_suffix)->default_value(".ttf"), "suffix for extracted font files")
        ("font-format", po::value<string>(&param.font_format)->default_value("truetype"), "format for extracted font files")

        ("debug", po::value<int>(&param.debug)->default_value(0), "output debug information")
        ("clean-tmp", po::value<int>(&param.clean_tmp)->default_value(1), "clean temporary files after processing")
        ;

    opt_hidden.add_options()
        ("inputfilename", po::value<string>(&param.input_filename)->default_value(""), "")
        ("outputfilename", po::value<string>(&param.output_filename)->default_value(""), "")
        ;

    opt_positional.add("inputfilename", 1).add("outputfilename",1);

    opt_all.add(opt_visible).add(opt_hidden);

    try {
        po::variables_map opt_vm;
        po::store(po::command_line_parser(argc, argv).options(opt_all).positional(opt_positional).run()
                , opt_vm);
        po::notify(opt_vm);
        return opt_vm;
    }
    catch(...) {
        show_usage();
        abort();
    }
}

int main(int argc, char **argv)
{
    auto opt_map = parse_options(argc, argv);
    if (opt_map.count("version") || opt_map.count("help") || (param.input_filename == ""))
    {
        show_usage();
        abort();
    }

    //prepare the directories
    char buf[] = "/tmp/pdf2htmlEX-XXXXXX";
    auto p = mkdtemp(buf);
    if(p == nullptr)
    {
        cerr << "Cannot create temp directory" << endl;
        abort();
    }
    param.tmp_dir = buf;

    if(param.debug)
        cerr << "temporary dir: " << (param.tmp_dir) << endl;

    try
    {
        create_directories(param.dest_dir);
    }
    catch (const string & s)
    {
        cerr << s << endl;
        abort();
    }

    // read config file
    globalParams = new GlobalParams();

    // open PDF file
    ownerPW = (param.owner_password == "") ? (nullptr) : (new GooString(param.owner_password.c_str()));
    userPW = (param.user_password == "") ? (nullptr) : (new GooString(param.user_password.c_str()));
    fileName = new GooString(param.input_filename.c_str());

    doc = PDFDocFactory().createPDFDoc(*fileName, ownerPW, userPW);

    delete userPW;
    delete ownerPW;

    if (!doc->isOk()) {
        goto error;
    }

    // check for copy permission
    if (!doc->okToCopy()) {
        error(errNotAllowed, -1, "Copying of text from this document is not allowed.");
        goto error;
    }

    param.first_page = min(max(param.first_page, 1), doc->getNumPages());
    param.last_page = min(max(param.last_page, param.first_page), doc->getNumPages());

    if(param.output_filename == "")
    {
        const string s = get_filename(param.input_filename);

        if(get_suffix(param.input_filename) == ".pdf")
        {
            param.output_filename = s.substr(0, s.size() - 4) + ".html";
        }
        else
        {
            param.output_filename = s + ".html";
        }
    }

    htmlOut = new HTMLRenderer(&param);
    htmlOut->process(doc);
    delete htmlOut;

    finished = true;

    // clean up
error:
    if(doc) delete doc;
    delete fileName;
    if(globalParams) delete globalParams;

    // check for memory leaks
    Object::memCheck(stderr);
    gMemReport(stderr);

    if(!finished)
        abort();

    return 0;
}
