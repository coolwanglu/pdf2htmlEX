/*
 * general.cc
 *
 * Handling general stuffs
 *
 * Copyright (C) 2012,2013,2014 Lu Wang <coolwanglu@gmail.com>
 */

#include <cstdio>
#include <ostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <functional>

#include <GlobalParams.h>

#include "pdf2htmlEX-config.h"
#include "HTMLRenderer.h"
#include "HTMLTextLine.h"
#include "Base64Stream.h"

#include "BackgroundRenderer/BackgroundRenderer.h"

#include "util/namespace.h"
#include "util/ffw.h"
#include "util/math.h"
#include "util/path.h"
#include "util/css_const.h"
#include "util/encoding.h"

namespace pdf2htmlEX {

using std::fixed;
using std::flush;
using std::ostream;
using std::max;
using std::min_element;
using std::vector;
using std::abs;
using std::cerr;
using std::endl;

HTMLRenderer::HTMLRenderer(const Param & param)
    :OutputDev()
    ,param(param)
    ,html_text_page(param, all_manager)
    ,preprocessor(param)
    ,tmp_files(param)
    ,tracer(param)
{
    if(!(param.debug))
    {
        //disable error messages of poppler
        globalParams->setErrQuiet(gTrue);
    }

    ffw_init(param.debug);

    cur_mapping.resize(0x10000);
    cur_mapping2.resize(0x100);
    width_list.resize(0x10000);

    /*
     * For these states, usually the error will not be accumulated
     * or may be handled well (whitespace_manager)
     * So we can set a large eps here
     */
    all_manager.vertical_align.set_eps(param.v_eps);
    all_manager.whitespace    .set_eps(param.h_eps);
    all_manager.left          .set_eps(param.h_eps);
    /*
     * For other states, we need accurate values
     * optimization will be done separately
     */
    all_manager.font_size   .set_eps(EPS);
    all_manager.letter_space.set_eps(EPS);
    all_manager.word_space  .set_eps(EPS);
    all_manager.height      .set_eps(EPS);
    all_manager.width       .set_eps(EPS);
    all_manager.bottom      .set_eps(EPS);

    tracer.on_char_drawn =
            [this](double * box) { covered_text_detector.add_char_bbox(box); };
    tracer.on_char_clipped =
            [this](double * box, bool partial) { covered_text_detector.add_char_bbox_clipped(box, partial); };
    tracer.on_non_char_drawn =
            [this](double * box) { covered_text_detector.add_non_char_bbox(box); };
}

HTMLRenderer::~HTMLRenderer()
{
    ffw_finalize();
}

void HTMLRenderer::process(PDFDoc *doc)
{
    cur_doc = doc;
    cur_catalog = doc->getCatalog();
    xref = doc->getXRef();

    pre_process(doc);

    ///////////////////
    // Process pages

    if(param.process_nontext)
    {
        bg_renderer = BackgroundRenderer::getBackgroundRenderer(param.bg_format, this, param);
        if(!bg_renderer)
            throw "Cannot initialize background renderer, unsupported format";
        bg_renderer->init(doc);

        fallback_bg_renderer = BackgroundRenderer::getFallbackBackgroundRenderer(this, param);
        if (fallback_bg_renderer)
            fallback_bg_renderer->init(doc);
    }

    int page_count = (param.last_page - param.first_page + 1);
    for(int i = param.first_page; i <= param.last_page ; ++i)
    {
        if (param.tmp_file_size_limit != -1 && tmp_files.get_total_size() > param.tmp_file_size_limit * 1024) {
            if(param.quiet == 0)
                cerr << "Stop processing, reach max size\n";
            break;
        }

        if (param.quiet == 0)
            cerr << "Working: " << (i-param.first_page) << "/" << page_count << '\r' << flush;

        if(param.split_pages)
        {
            // copy the string out, since we will reuse the buffer soon
            string filled_template_filename = (char*)str_fmt(param.page_filename.c_str(), i);
            auto page_fn = str_fmt("%s/%s", param.dest_dir.c_str(), filled_template_filename.c_str());
            f_curpage = new ofstream((char*)page_fn, ofstream::binary);
            if(!(*f_curpage))
                throw string("Cannot open ") + (char*)page_fn + " for writing";
            set_stream_flags((*f_curpage));

            cur_page_filename = filled_template_filename;
        }

        doc->displayPage(this, i,
                text_zoom_factor() * DEFAULT_DPI, text_zoom_factor() * DEFAULT_DPI,
                0,
                (!(param.use_cropbox)),
                true,  // crop
                false, // printing
                nullptr, nullptr, nullptr, nullptr);

        if(param.split_pages)
        {
            delete f_curpage;
            f_curpage = nullptr;
        }
    }
    if(page_count >= 0 && param.quiet == 0)
        cerr << "Working: " << page_count << "/" << page_count;

    if(param.quiet == 0)
        cerr << endl;

    ////////////////////////
    // Process Outline
    if(param.process_outline)
        process_outline();

    post_process();

    bg_renderer = nullptr;
    fallback_bg_renderer = nullptr;

    if(param.quiet == 0)
        cerr << endl;
}

void HTMLRenderer::setDefaultCTM(double *ctm)
{
    memcpy(default_ctm, ctm, sizeof(default_ctm));
}

void HTMLRenderer::startPage(int pageNum, GfxState *state, XRef * xref)
{
    covered_text_detector.reset();
    tracer.reset(state);

    this->pageNum = pageNum;

    html_text_page.set_page_size(state->getPageWidth(), state->getPageHeight());

    reset_state();
}

void HTMLRenderer::endPage() {
    long long wid = all_manager.width.install(html_text_page.get_width());
    long long hid = all_manager.height.install(html_text_page.get_height());

    (*f_curpage)
        << "<div id=\"" << CSS::PAGE_FRAME_CN << pageNum
            << "\" class=\"" << CSS::PAGE_FRAME_CN
            << " " << CSS::WIDTH_CN << wid
            << " " << CSS::HEIGHT_CN << hid
            << "\" data-page-no=\"" << pageNum << "\">"
        << "<div class=\"" << CSS::PAGE_CONTENT_BOX_CN
            << " " << CSS::PAGE_CONTENT_BOX_CN << pageNum
            << " " << CSS::WIDTH_CN << wid
            << " " << CSS::HEIGHT_CN << hid
            << "\">";

    /*
     * When split_pages is on, f_curpage points to the current page file
     * and we want to output empty frames in f_pages.fs
     */
    if(param.split_pages)
    {
        f_pages.fs
            << "<div id=\"" << CSS::PAGE_FRAME_CN << pageNum
                << "\" class=\"" << CSS::PAGE_FRAME_CN
                << " " << CSS::WIDTH_CN << wid
                << " " << CSS::HEIGHT_CN << hid
                << "\" data-page-no=\"" << pageNum
                << "\" data-page-url=\"";

        writeAttribute(f_pages.fs, cur_page_filename);
        f_pages.fs << "\">";
    }

    if(param.process_nontext)
    {
        if (bg_renderer->render_page(cur_doc, pageNum))
        {
            bg_renderer->embed_image(pageNum);
        }
        else if (fallback_bg_renderer)
        {
            if (fallback_bg_renderer->render_page(cur_doc, pageNum))
                fallback_bg_renderer->embed_image(pageNum);
        }
    }

    // dump all text
    html_text_page.dump_text(*f_curpage);
    html_text_page.dump_css(f_css.fs);
    html_text_page.clear();

    // process form
    if(param.process_form)
        process_form(*f_curpage);
    
    // process links before the page is closed
    cur_doc->processLinks(this, pageNum);

    // close box
    (*f_curpage) << "</div>";

    // dump info for js
    // TODO: create a function for this
    // BE CAREFUL WITH ESCAPES
    {
        (*f_curpage) << "<div class=\"" << CSS::PAGE_DATA_CN << "\" data-data='{";

        //default CTM
        (*f_curpage) << "\"ctm\":[";
        for(int i = 0; i < 6; ++i)
        {
            if(i > 0) (*f_curpage) << ",";
            (*f_curpage) << round(default_ctm[i]);
        }
        (*f_curpage) << "]";

        (*f_curpage) << "}'></div>";
    }

    // close page
    (*f_curpage) << "</div>" << endl;

    if(param.split_pages)
    {
        f_pages.fs << "</div>" << endl;
    }
}

void HTMLRenderer::pre_process(PDFDoc * doc)
{
    preprocessor.process(doc);

    /*
     * determine scale factors
     */
    {
        vector<double> zoom_factors;

        if(is_positive(param.zoom))
        {
            zoom_factors.push_back(param.zoom);
        }

        if(is_positive(param.fit_width))
        {
            zoom_factors.push_back((param.fit_width) / preprocessor.get_max_width());
        }

        if(is_positive(param.fit_height))
        {
            zoom_factors.push_back((param.fit_height) / preprocessor.get_max_height());
        }

        double zoom = (zoom_factors.empty() ? 1.0 : (*min_element(zoom_factors.begin(), zoom_factors.end())));

        text_scale_factor1 = max<double>(zoom, param.font_size_multiplier);
        text_scale_factor2 = zoom / text_scale_factor1;
    }

    // we may output utf8 characters, so always use binary
    {
        /*
         * If embed-css
         * we have to keep the generated css file into a temporary place
         * and embed it into the main html later
         *
         * otherwise
         * leave it in param.dest_dir
         */

        auto fn = (param.embed_css)
            ? str_fmt("%s/__css", param.tmp_dir.c_str())
            : str_fmt("%s/%s", param.dest_dir.c_str(), param.css_filename.c_str());

        if(param.embed_css)
            tmp_files.add((char*)fn);

        f_css.path = (char*)fn;
        f_css.fs.open(f_css.path, ofstream::binary);
        if(!f_css.fs)
            throw string("Cannot open ") + (char*)fn + " for writing";
        set_stream_flags(f_css.fs);
    }

    if (param.process_outline)
    {
        /*
         * The logic for outline is similar to css
         */

        auto fn = (param.embed_outline)
            ? str_fmt("%s/__outline", param.tmp_dir.c_str())
            : str_fmt("%s/%s", param.dest_dir.c_str(), param.outline_filename.c_str());

        if(param.embed_outline)
            tmp_files.add((char*)fn);

        f_outline.path = (char*)fn;
        f_outline.fs.open(f_outline.path, ofstream::binary);
        if(!f_outline.fs)
            throw string("Cannot open") + (char*)fn + " for writing";

        // might not be necessary
        set_stream_flags(f_outline.fs);
    }

    {
        /*
         * we have to keep the html file for pages into a temporary place
         * because we'll have to embed css before it
         *
         * Otherwise just generate it
         */
        auto fn = str_fmt("%s/__pages", param.tmp_dir.c_str());
        tmp_files.add((char*)fn);

        f_pages.path = (char*)fn;
        f_pages.fs.open(f_pages.path, ofstream::binary);
        if(!f_pages.fs)
            throw string("Cannot open ") + (char*)fn + " for writing";
        set_stream_flags(f_pages.fs);
    }

    if(param.split_pages)
    {
        f_curpage = nullptr;
    }
    else
    {
        f_curpage = &f_pages.fs;
    }
}

void HTMLRenderer::post_process(void)
{
    dump_css();
    
    // close files if they opened
    if (param.process_outline)
    {
        f_outline.fs.close();
    }
    f_pages.fs.close();
    f_css.fs.close();

    // build the main HTML file
    ofstream output;
    {
        auto fn = str_fmt("%s/%s", param.dest_dir.c_str(), param.output_filename.c_str());
        output.open((char*)fn, ofstream::binary);
        if(!output)
            throw string("Cannot open ") + (char*)fn + " for writing";
        set_stream_flags(output);
    }

    // apply manifest
    ifstream manifest_fin((char*)str_fmt("%s/%s", param.data_dir.c_str(), MANIFEST_FILENAME.c_str()), ifstream::binary);
    if(!manifest_fin)
        throw "Cannot open the manifest file";

    bool embed_string = false;
    string line;
    long line_no = 0;
    while(getline(manifest_fin, line))
    {
        // trim space at both sides
        {
            static const char * whitespaces = " \t\n\v\f\r";
            auto idx1 = line.find_first_not_of(whitespaces);
            if(idx1 == string::npos)
            {
                line.clear();
            }
            else
            {
                auto idx2 = line.find_last_not_of(whitespaces);
                assert(idx2 >= idx1);
                line = line.substr(idx1, idx2 - idx1 + 1);
            }
        }

        ++line_no;

        if(line == "\"\"\"")
        {
            embed_string = !embed_string;
            continue;
        }

        if(embed_string)
        {
            output << line << endl;
            continue;
        }

        if(line.empty() || line[0] == '#')
            continue;


        if(line[0] == '@')
        {
            embed_file(output, param.data_dir + "/" + line.substr(1), "", true);
            continue;
        }

        if(line[0] == '$')
        {
            if(line == "$css")
            {
                embed_file(output, f_css.path, ".css", false);
            }
            else if (line == "$outline")
            {
                if (param.process_outline && param.embed_outline)
                {
                    ifstream fin(f_outline.path, ifstream::binary);
                    if(!fin)
                        throw "Cannot open outline for reading";
                    output << fin.rdbuf();
                    output.clear(); // output will set fail big if fin is empty
                }
            }
            else if (line == "$pages")
            {
                ifstream fin(f_pages.path, ifstream::binary);
                if(!fin)
                    throw "Cannot open pages for reading";
                output << fin.rdbuf();
                output.clear(); // output will set fail bit if fin is empty
            }
            else
            {
                cerr << "Warning: manifest line " << line_no << ": Unknown content \"" << line << "\"" << endl;
            }
            continue;
        }

        cerr << "Warning: unknown line in manifest: " << line << endl;
    }
}

void HTMLRenderer::set_stream_flags(std::ostream & out)
{
    // we output all ID's in hex
    // browsers are not happy with scientific notations
    out << hex << fixed;
}

void HTMLRenderer::dump_css (void)
{
    all_manager.transform_matrix.dump_css(f_css.fs);
    all_manager.vertical_align  .dump_css(f_css.fs);
    all_manager.letter_space    .dump_css(f_css.fs);
    all_manager.stroke_color    .dump_css(f_css.fs);
    all_manager.word_space      .dump_css(f_css.fs);
    all_manager.whitespace      .dump_css(f_css.fs);
    all_manager.fill_color      .dump_css(f_css.fs);
    all_manager.font_size       .dump_css(f_css.fs);
    all_manager.bottom          .dump_css(f_css.fs);
    all_manager.height          .dump_css(f_css.fs);
    all_manager.width           .dump_css(f_css.fs);
    all_manager.left            .dump_css(f_css.fs);
    all_manager.bgimage_size    .dump_css(f_css.fs);

    // print css
    if(param.printing)
    {
        double ps = print_scale();
        f_css.fs << CSS::PRINT_ONLY << "{" << endl;
        all_manager.transform_matrix.dump_print_css(f_css.fs, ps);
        all_manager.vertical_align  .dump_print_css(f_css.fs, ps);
        all_manager.letter_space    .dump_print_css(f_css.fs, ps);
        all_manager.stroke_color    .dump_print_css(f_css.fs, ps);
        all_manager.word_space      .dump_print_css(f_css.fs, ps);
        all_manager.whitespace      .dump_print_css(f_css.fs, ps);
        all_manager.fill_color      .dump_print_css(f_css.fs, ps);
        all_manager.font_size       .dump_print_css(f_css.fs, ps);
        all_manager.bottom          .dump_print_css(f_css.fs, ps);
        all_manager.height          .dump_print_css(f_css.fs, ps);
        all_manager.width           .dump_print_css(f_css.fs, ps);
        all_manager.left            .dump_print_css(f_css.fs, ps);
        all_manager.bgimage_size    .dump_print_css(f_css.fs, ps);
        f_css.fs << "}" << endl;
    }
}

void HTMLRenderer::embed_file(ostream & out, const string & path, const string & type, bool copy)
{
    string fn = get_filename(path);
    string suffix = (type == "") ? get_suffix(fn) : type;

    auto iter = EMBED_STRING_MAP.find(suffix);
    if(iter == EMBED_STRING_MAP.end())
    {
        cerr << "Warning: unknown suffix: " << suffix << endl;
        return;
    }

    const auto & entry = iter->second;

    if(param.*(entry.embed_flag))
    {
        ifstream fin(path, ifstream::binary);
        if(!fin)
            throw string("Cannot open file ") + path + " for embedding";
        out << entry.prefix_embed;

        if(entry.base64_encode)
        {
            out << Base64Stream(fin);
        }
        else
        {
            out << endl << fin.rdbuf();
        }
        out.clear(); // out will set fail big if fin is empty
        out << entry.suffix_embed << endl;
    }
    else
    {
        out << entry.prefix_external;
        writeAttribute(out, fn);
        out << entry.suffix_external << endl;

        if(copy)
        {
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw string("Cannot copy file: ") + path;
            auto out_path = param.dest_dir + "/" + fn;
            ofstream out(out_path, ofstream::binary);
            if(!out)
                throw string("Cannot open file ") + path + " for embedding";
            out << fin.rdbuf();
            out.clear(); // out will set fail big if fin is empty
        }
    }
}

const std::string HTMLRenderer::MANIFEST_FILENAME = "manifest";

}// namespace pdf2htmlEX
