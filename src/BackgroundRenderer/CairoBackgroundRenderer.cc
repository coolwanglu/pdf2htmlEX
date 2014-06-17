/*
 * CairoBackgroundRenderer.cc
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <string>
#include <fstream>


#include "pdf2htmlEX-config.h"

#include "Base64Stream.h"

#if ENABLE_SVG

#include "CairoBackgroundRenderer.h"
#include "SplashBackgroundRenderer.h"

namespace pdf2htmlEX {

using std::string;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::unordered_map;

CairoBackgroundRenderer::CairoBackgroundRenderer(HTMLRenderer * html_renderer, const Param & param)
    : CairoOutputDev()
    , html_renderer(html_renderer)
    , param(param)
    , surface(nullptr)
{ }

CairoBackgroundRenderer::~CairoBackgroundRenderer()
{
    for(auto itr = bitmaps_ref_count.begin(); itr != bitmaps_ref_count.end(); ++itr)
    {
        if (itr->second == 0)
        {
            html_renderer->tmp_files.add(this->build_bitmap_path(itr->first));
        }
    }
}

void CairoBackgroundRenderer::drawChar(GfxState *state, double x, double y,
        double dx, double dy,
        double originX, double originY,
        CharCode code, int nBytes, Unicode *u, int uLen)
{
    // draw characters as image when
    // - in fallback mode
    // - OR there is special filling method
    // - OR using a writing mode font
    // - OR using a Type 3 font while param.process_type3 is not enabled
    if((param.fallback)
       || ( (state->getFont()) 
            && ( (state->getFont()->getWMode())
                 || ((state->getFont()->getType() == fontType3) && (!param.process_type3))
               )
          )
      )
    {
        CairoOutputDev::drawChar(state,x,y,dx,dy,originX,originY,code,nBytes,u,uLen);
    }
}

void CairoBackgroundRenderer::init(PDFDoc * doc)
{
    startDoc(doc);
}

static GBool annot_cb(Annot *, void * pflag) {
    return (*((bool*)pflag)) ? gTrue : gFalse;
};

bool CairoBackgroundRenderer::render_page(PDFDoc * doc, int pageno)
{
    double page_width;
    double page_height;
    if(param.use_cropbox)
    {
        page_width = doc->getPageCropWidth(pageno);
        page_height = doc->getPageCropHeight(pageno);
    }
    else
    {
        page_width = doc->getPageMediaWidth(pageno);
        page_height = doc->getPageMediaHeight(pageno);
    }

    string fn = (char*)html_renderer->str_fmt("%s/bg%x.svg", (param.embed_image ? param.tmp_dir : param.dest_dir).c_str(), pageno);
    if(param.embed_image)
        html_renderer->tmp_files.add(fn);

    surface = cairo_svg_surface_create(fn.c_str(), page_width * param.h_dpi / DEFAULT_DPI, page_height * param.v_dpi / DEFAULT_DPI);
    cairo_svg_surface_restrict_to_version(surface, CAIRO_SVG_VERSION_1_2);
    cairo_surface_set_fallback_resolution(surface, param.h_dpi, param.v_dpi);

    cairo_t * cr = cairo_create(surface);
    setCairo(cr);

    bitmaps_in_current_page.clear();

    bool process_annotation = param.process_annotation;
    doc->displayPage(this, pageno, param.h_dpi, param.v_dpi,
            0, 
            (!(param.use_cropbox)),
            false, 
            false,
            nullptr, nullptr, &annot_cb, &process_annotation);

    setCairo(nullptr);
    
    {
        auto status = cairo_status(cr);
        cairo_destroy(cr);
        if(status)
            throw string("Cairo error: ") + cairo_status_to_string(status);
    }

    cairo_surface_finish(surface);
    {
        auto status = cairo_surface_status(surface);
        cairo_surface_destroy(surface);
        surface = nullptr;
        if(status)
            throw string("Error in cairo: ") + cairo_status_to_string(status);
    }

    //check node count in the svg file, fall back to bitmap_renderer if necessary.
    if (param.svg_node_count_limit >= 0)
    {
        int n = 0;
        char c;
        ifstream svgfile(fn);
        //count of '<' in the file should be an approximation of node count.
        while(svgfile >> c)
        {
            if (c == '<')
                ++n;
            if (n > param.svg_node_count_limit)
            {
                html_renderer->tmp_files.add(fn);
                return false;
            }
        }
    }

    // the svg file is actually used, so add its bitmaps' ref count.
    for (auto itr = bitmaps_in_current_page.begin(); itr != bitmaps_in_current_page.end(); itr++)
        ++bitmaps_ref_count[*itr];

    return true;
}

void CairoBackgroundRenderer::embed_image(int pageno)
{
    auto & f_page = *(html_renderer->f_curpage);
    
    // SVGs introduced by <img> or background-image can't have external resources;
    // SVGs introduced by <embed> and <object> can, but they are more expensive for browsers.
    // So we use <img> if the SVG contains no external bitmaps, and use <embed> otherwise.
    // See also:
    //   https://developer.mozilla.org/en-US/docs/Web/SVG/SVG_as_an_Image
    //   http://stackoverflow.com/questions/4476526/do-i-use-img-object-or-embed-for-svg-files

    if (param.svg_embed_bitmap || bitmaps_in_current_page.empty())
        f_page << "<img";
    else
        f_page << "<embed";

    f_page << " class=\"" << CSS::FULL_BACKGROUND_IMAGE_CN
        << "\" alt=\"\" src=\"";

    if(param.embed_image)
    {
        auto path = html_renderer->str_fmt("%s/bg%x.svg", param.tmp_dir.c_str(), pageno);
        ifstream fin((char*)path, ifstream::binary);
        if(!fin)
            throw string("Cannot read background image ") + (char*)path;
        f_page << "data:image/svg+xml;base64," << Base64Stream(fin);
    }
    else
    {
        f_page << (char*)html_renderer->str_fmt("bg%x.svg", pageno);
    }
    f_page << "\"/>";
}

string CairoBackgroundRenderer::build_bitmap_path(int id)
{
    // "o" for "PDF Object"
    return string(html_renderer->str_fmt("%s/o%d.jpg", param.dest_dir.c_str(), id));
}
// Override CairoOutputDev::setMimeData() and dump bitmaps in SVG to external files.
void CairoBackgroundRenderer::setMimeData(Stream *str, Object *ref, cairo_surface_t *image)
{
    if (param.svg_embed_bitmap)
    {
        CairoOutputDev::setMimeData(str, ref, image);
        return;
    }

    // TODO dump bitmaps in other formats.
    if (str->getKind() != strDCT)
        return;

    // TODO inline image?
    if (ref == nullptr || !ref->isRef())
        return;

    int imgId = ref->getRef().num;
    auto uri = strdup((char*) html_renderer->str_fmt("o%d.jpg", imgId));
    auto st = cairo_surface_set_mime_data(image, CAIRO_MIME_TYPE_URI,
        (unsigned char*) uri, strlen(uri), free, uri);
    if (st)
    {
        free(uri);
        return;
    }
    bitmaps_in_current_page.push_back(imgId);

    if(bitmaps_ref_count.find(imgId) != bitmaps_ref_count.end())
        return;

    bitmaps_ref_count[imgId] = 0;

    char *strBuffer;
    int len;
    if (getStreamData(str->getNextStream(), &strBuffer, &len))
    {
        ofstream imgfile(build_bitmap_path(imgId), ofstream::binary);
        imgfile.write(strBuffer, len);
        free(strBuffer);
    }
}

} // namespace pdf2htmlEX

#endif // ENABLE_SVG

