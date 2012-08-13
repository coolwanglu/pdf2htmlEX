//========================================================================
// pdftohtmlEX.cc
//
// Copyright (C) 2011 by Hongliang TIAN(tatetian@gmail.com)
// Copyright (C) 2012 by Lu Wang coolwanglu<at>gmail.com
//========================================================================
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <string>
#include <limits>
#include <iostream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <goo/GooString.h>

#include "Object.h"
#include "PDFDoc.h"
#include "PDFDocFactory.h"
#include "HTMLRenderer.h"
#include "GlobalParams.h"
#include "Error.h"
#include "DateInfo.h"

#include "Param.h"

#define PDFTOHTMLEX_VERSION "0.1"

namespace po = boost::program_options;
using namespace std;

Param param;

// variables
PDFDoc *doc = nullptr;
GooString *fileName = nullptr;
GooString *ownerPW, *userPW;

GooString *docTitle = nullptr;
GooString *author = nullptr, *keywords = nullptr, *subject = nullptr, *date = nullptr;

HTMLRenderer *htmlOut = nullptr;

int finished = -1;

po::options_description opt_visible("Options"), opt_hidden, opt_all;
po::positional_options_description opt_positional;

//====================helper functions=========================================
/*
static GooString* getInfoString(Dict *infoDict, char *key) {
    Object obj;
    GooString *s1 = nullptr;

    if (infoDict->lookup(key, &obj)->isString()) {
        s1 = new GooString(obj.getString());
    }
    obj.free();
    return s1;
}

static GooString* getInfoDate(Dict *infoDict, char *key) {
    Object obj;
    char *s;
    int year, mon, day, hour, min, sec, tz_hour, tz_minute;
    char tz;
    struct tm tmStruct;
    GooString *result = nullptr;
    char buf[256];

    if (infoDict->lookup(key, &obj)->isString()) {
        s = obj.getString()->getCString();
        // TODO do something with the timezone info
        if ( parseDateString( s, &year, &mon, &day, &hour, &min, &sec, &tz, &tz_hour, &tz_minute ) ) {
            tmStruct.tm_year = year - 1900;
            tmStruct.tm_mon = mon - 1;
            tmStruct.tm_mday = day;
            tmStruct.tm_hour = hour;
            tmStruct.tm_min = min;
            tmStruct.tm_sec = sec;
            tmStruct.tm_wday = -1;
            tmStruct.tm_yday = -1;
            tmStruct.tm_isdst = -1;
            mktime(&tmStruct); // compute the tm_wday and tm_yday fields
            if (strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S+00:00", &tmStruct)) {
                result = new GooString(buf);
            } else {
                result = new GooString(s);
            }
        } else {
            result = new GooString(s);
        }
    }
    obj.free();
    return result;
}
*/

void show_usage(void)
{
    cerr << "pdftohtmlEX version " << PDFTOHTMLEX_VERSION << endl;
    cerr << endl;
    cerr << "Copyright 2011 Hongliang Tian (tatetian@gmail.com)" << endl;
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
        ("first-page,f", po::value<int>(&param.first_page)->default_value(1), "first page to process")
        ("last-page,l", po::value<int>(&param.last_page)->default_value(numeric_limits<int>::max()), "last page to process")
        ("version,v", "show copyright and version info")
        ("metadata,m", "show the document meta data in JSON")
        ("owner-password,o", po::value<string>(&param.owner_password)->default_value(""), "owner password (for encrypted files)")
        ("user-password,u", po::value<string>(&param.user_password)->default_value(""), "user password (for encrypted files)")
        ("hdpi", po::value<double>(&param.h_dpi)->default_value(72.0), "horizontal DPI for text")
        ("vdpi", po::value<double>(&param.v_dpi)->default_value(72.0), "vertical DPI for text")
        ("hdpi2", po::value<double>(&param.h_dpi2)->default_value(144.0), "horizontal DPI for non-text")
        ("vdpi2", po::value<double>(&param.v_dpi2)->default_value(144.0), "vertical DPI for non-text")
        ("heps", po::value<double>(&param.h_eps)->default_value(1.0), "max tolerated horizontal offset (in pixels)")
        ("veps", po::value<double>(&param.v_eps)->default_value(1.0), "max tolerated vertical offset (in pixels)")
        ("process-nontext", po::value<int>(&param.process_nontext)->default_value(1), "process nontext objects")
        ("debug", po::value<int>(&param.debug)->default_value(0), "output debug information")
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
        exit(-1);
    }
}


//====================entry point==============================================
int main(int argc, char **argv)
{
    auto opt_map = parse_options(argc, argv);
    if (opt_map.count("version") || opt_map.count("help") || (param.input_filename == ""))
    {
        show_usage();
        return -1;
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

    /*
    // get meta info
    doc->getDocInfo(&info);
    if (info.isDict()) {
        docTitle = getInfoString(info.getDict(), "Title");
        author = getInfoString(info.getDict(), "Author");
        keywords = getInfoString(info.getDict(), "Keywords");
        subject = getInfoString(info.getDict(), "Subject");
        date = getInfoDate(info.getDict(), "ModDate");
        if( !date )
            date = getInfoDate(info.getDict(), "CreationDate");
    }
    info.free();
    if( !docTitle ) docTitle = fileName->copy();
    */

    if(param.output_filename == "")
    {
        const auto & s = param.input_filename;
        if(boost::algorithm::ends_with(s, ".pdf"))
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

    {
        /*
        escapeHTMLString(docTitle);
        if(author) escapeHTMLString(author);
        if(date) escapeHTMLString(date);

        printf("{\"doc_id\": \"\", \"title\":\"%s\", \"author\":\"%s\",\"mod_date\":\"%s\",\n",
                docTitle->getCString(), 
                author? author->getCString():"", 
                date? date->getCString():"");
        printf("\"pages\":[\n");
        */
    }

    delete htmlOut;
    delete docTitle;
    if( author ) delete author;
    if( keywords ) delete keywords;
    if( subject ) delete subject;
    if( date ) delete date;

    finished = 0;

    // clean up
error:
    if(doc) delete doc;
    delete fileName;
    if(globalParams) delete globalParams;

    // check for memory leaks
    Object::memCheck(stderr);
    gMemReport(stderr);

    return finished;
}
