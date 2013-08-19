// pdf2htmlEX.cc
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
#ifndef _PATH_TMP
#define _PATH_TMP "/tmp"
#endif
extern "C" {
//https://github.com/witwall/mktemp	
#include <mktemp_config.h>
#include <mktemp.h>
}
#include <libgen.h>

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

std::string data_dir;

void deprecated_single_html(const char * dummy = nullptr)
{
    cerr << "--single_html is deprecated. Use `--embed CFIJO` instead." << endl;
    exit(EXIT_FAILURE);
}

void removed_remove_unsed_glyph(const char * dummy = nullptr)
{
    cerr << "--remove-unsed-glyph is removed. Use a PDF optimization tool instead." << endl;
    exit(EXIT_FAILURE);
}

void show_usage_and_exit(const char * dummy = nullptr)
{
    cerr << "Usage: pdf2htmlEX [options] <input.pdf> [<output.html>]" << endl;
    argparser.show_usage(cerr);
    exit(EXIT_FAILURE);
}

void show_version_and_exit(const char * dummy = nullptr)
{
    cerr << "pdf2htmlEX version " << PDF2HTMLEX_VERSION << endl;
    cerr << "Copyright 2012,2013 Lu Wang <coolwanglu@gmail.com>" << endl;
    cerr << "Libraries: ";
    cerr << "poppler " << POPPLER_VERSION << ", ";
    cerr << "libfontforge " << ffw_get_version() << endl;
    cerr << "Default data-dir: " << data_dir <<endl;//PDF2HTMLEX_DATA_PATH << endl;
    exit(EXIT_SUCCESS);
}

void embed_parser (const char * str)
{
    while(true)
    {
        switch(*str)
        {
            case '\0': return; break;
            case 'c': param.embed_css = 0; break;
            case 'C': param.embed_css = 1; break;
            case 'f': param.embed_font = 0; break;
            case 'F': param.embed_font = 1; break;
            case 'i': param.embed_image = 0; break;
            case 'I': param.embed_image = 1; break;
            case 'j': param.embed_javascript = 0; break;
            case 'J': param.embed_javascript = 1; break;
            case 'o': param.embed_outline = 0; break;
            case 'O': param.embed_outline = 1; break;
            default:
                cerr << "Unknown character `" << (*str) << "` for --embed" << endl;
                break;
        }
        ++ str;
    }
}

void parse_options (int argc, char **argv)
{
    argparser
        // pages
        .add("first-page,f", &param.first_page, 1, "first page to convert")
        .add("last-page,l", &param.last_page, numeric_limits<int>::max(), "last page to convert")
        
        // dimensions
        .add("zoom", &param.zoom, 0, "zoom ratio", true)
        .add("fit-width", &param.fit_width, 0, "fit width to <fp> pixels", true) 
        .add("fit-height", &param.fit_height, 0, "fit height to <fp> pixels", true)
        .add("use-cropbox", &param.use_cropbox, 1, "use CropBox instead of MediaBox")
        .add("hdpi", &param.h_dpi, 144.0, "horizontal resolution for graphics in DPI")
        .add("vdpi", &param.v_dpi, 144.0, "vertical resolution for graphics in DPI")
        
        // output files
        .add("embed", "specify which elements should be embedded into output", embed_parser, true)
        .add("embed-css", &param.embed_css, 1, "embed CSS files into output")
        .add("embed-font", &param.embed_font, 1, "embed font files into output")
        .add("embed-image", &param.embed_image, 1, "embed image files into output")
        .add("embed-javascript", &param.embed_javascript, 1, "embed JavaScript files into output")
        .add("embed-outline", &param.embed_outline, 1, "embed outlines into output")
        .add("split-pages", &param.split_pages, 0, "split pages into separate files")
        .add("dest-dir", &param.dest_dir, ".", "specify destination directory")
        .add("css-filename", &param.css_filename, "", "filename of the generated css file")
        .add("page-filename", &param.page_filename, "", "filename template for splitted pages ")
        .add("outline-filename", &param.outline_filename, "", "filename of the generated outline file")
        .add("process-nontext", &param.process_nontext, 1, "render graphics in addition to text")
        .add("process-outline", &param.process_outline, 1, "show outline in HTML")
        .add("printing", &param.printing, 1, "enable printing support")
        .add("fallback", &param.fallback, 0, "output in fallback mode")
        
        // fonts
        .add("embed-external-font", &param.embed_external_font, 1, "embed local match for external fonts")
        .add("font-suffix", &param.font_suffix, ".ttf", "suffix for embedded font files (.ttf,.otf,.woff,.svg)")
        .add("decompose-ligature", &param.decompose_ligature, 0, "decompose ligatures, such as \uFB01 -> fi")
        .add("auto-hint", &param.auto_hint, 0, "use fontforge autohint on fonts without hints")
        .add("external-hint-tool", &param.external_hint_tool, "", "external tool for hinting fonts (overrides --auto-hint)")
        .add("stretch-narrow-glyph", &param.stretch_narrow_glyph, 0, "stretch narrow glyphs instead of padding them")
        .add("squeeze-wide-glyph", &param.squeeze_wide_glyph, 1, "shrink wide glyphs instead of truncating them")
        .add("override-fstype", &param.override_fstype, 0, "clear the fstype bits in TTF/OTF fonts")
        
        // text
        .add("heps", &param.h_eps, 1.0, "horizontal threshold for merging text, in pixels")
        .add("veps", &param.v_eps, 1.0, "vertical threshold for merging text, in pixels")
        .add("space-threshold", &param.space_threshold, (1.0/8), "word break threshold (threshold * em)")
        .add("font-size-multiplier", &param.font_size_multiplier, 4.0, "a value greater than 1 increases the rendering accuracy")
        .add("space-as-offset", &param.space_as_offset, 0, "treat space characters as offsets")
        .add("tounicode", &param.tounicode, 0, "how to handle ToUnicode CMaps (0=auto, 1=force, -1=ignore)")
        .add("optimize-text", &param.optimize_text, 0, "try to reduce the number of HTML elements used for text")
        
        // encryption
        .add("owner-password,o", &param.owner_password, "", "owner password (for encrypted files)", true)
        .add("user-password,u", &param.user_password, "", "user password (for encrypted files)", true)
        .add("no-drm", &param.no_drm, 0, "override document DRM settings")
        
        // misc.
        .add("clean-tmp", &param.clean_tmp, 1, "remove temporary files after conversion")
        .add("data-dir", &param.data_dir, data_dir/*PDF2HTMLEX_DATA_PATH*/, "specify data directory")
        // TODO: css drawings are hidden on print, for annot links, need to fix it for other drawings
//        .add("css-draw", &param.css_draw, 0, "[experimental and unsupported] CSS drawing")
        .add("debug", &param.debug, 0, "print debugging information")
        
        // meta
        .add("version,v", "print copyright and version info", &show_version_and_exit)
        .add("help,h", "print usage information", &show_usage_and_exit)

        // deprecated
        .add("single-html", "", &deprecated_single_html)
        .add("remove-unused-glyph", "", &removed_remove_unsed_glyph)
        
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

    std::stringstream ss;
    ss << dirname(argv[0]) << "\\data";
    data_dir = ss.str();
//    cout<<data_dir<<endl;

//    data_dir = argv[0] + "\\data";
    parse_options(argc, argv);

    if (param.input_filename == "")
    {
        show_usage_and_exit();
    }
    {
	char *cp, *_template ="pdf2htmlEX-XXXXXX", *tempfile, *prefix = _PATH_TMP;
	char buf [MAX_PATH];
        char separator='/';
	#ifdef _WIN32 
		separator='\\';
	#endif

        if(GetTempPath (MAX_PATH, buf) ==0)
		cp=getenv("TMPDIR");
	else
		cp=buf;

	//cp = getenv("TMPDIR");
        //cout <<"TMPDIR"<<cp<<endl;
	if (cp != NULL && *cp != '\0')
		prefix = cp;

	int plen = strlen(prefix);
	while (plen != 0 && prefix[plen - 1] == separator)
		plen--;

	tempfile = (char *)malloc(plen + 1 + strlen(_template) + 1);
	if (tempfile == NULL) {
		//if (!quiet)
		//	(void)fprintf(stderr,
		//	    "%s: cannot allocate memory\n", __progname);
        cerr << argv[0]<<": cannot allocate memory." << endl;
	exit(EXIT_FAILURE);
		//exit(1);
	}
	(void)memcpy(tempfile, prefix, plen);
	tempfile[plen] =  separator ;//'/';
	(void)strcpy(tempfile + plen + 1, _template);	/* SAFE */
	//cout << tempfile << endl;
        //char buf[] = "/tmp/pdf2htmlEX-XXXXXX";
        auto p = priv_mkdtemp(tempfile);
        if(p == nullptr)
        {
            cerr << "Cannot create temp directory" << endl;
            exit(EXIT_FAILURE);
        }
        param.tmp_dir = tempfile;
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
                param.output_filename = s.substr(0, s.size() - 4) + ".html";

            }
            else
            {
                param.output_filename = s + ".html";
            }
        }

        if(param.page_filename.empty())
        {
            const string s = get_filename(param.input_filename);
            if(get_suffix(param.input_filename) == ".pdf")
            {
                param.page_filename = s.substr(0, s.size() - 4) + "%d.page";
            }
            else
            {
                param.page_filename = s + "%d.page";
            }
            sanitize_filename(param.page_filename);
        }

		else
        {
            // Need to make sure we have a page number placeholder in the filename
            if(!sanitize_filename(param.page_filename))
            {
                // Inject the placeholder just before the file extension
                const string suffix = get_suffix(param.page_filename);
                param.page_filename = param.page_filename.substr(0, param.page_filename.size() - suffix.size()) + "%d" + suffix;
                sanitize_filename(param.page_filename);
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
