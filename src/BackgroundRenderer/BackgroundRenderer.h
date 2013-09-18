/*
 * Background renderer
 * Render all those things not supported as Image
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef BACKGROUND_RENDERER_H__
#define BACKGROUND_RENDERER_H__

#include <string>

class PDFDoc;

namespace pdf2htmlEX {

class Param;
class HTMLRenderer;
class BackgroundRenderer 
{
public:
    // return nullptr upon failure
    static BackgroundRenderer * getBackgroundRenderer(const std::string & format, HTMLRenderer * html_renderer, const Param & param);

    BackgroundRenderer() {}
    virtual ~BackgroundRenderer() {}

    virtual void init(PDFDoc * doc) = 0;
    virtual void render_page(PDFDoc * doc, int pageno) = 0;
    virtual void embed_image(int pageno) = 0;

};

} // namespace pdf2htmlEX

#endif //BACKGROUND_RENDERER_H__
