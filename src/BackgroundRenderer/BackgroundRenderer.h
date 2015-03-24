/*
 * Background renderer
 * Render all those things not supported as Image
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef BACKGROUND_RENDERER_H__
#define BACKGROUND_RENDERER_H__

#include <string>
#include <memory>

class PDFDoc;
class GfxState;
class OutputDev;

namespace pdf2htmlEX {

class Param;
class HTMLRenderer;
class BackgroundRenderer 
{
public:
    // return nullptr upon failure
    static std::unique_ptr<BackgroundRenderer> getBackgroundRenderer(const std::string & format, HTMLRenderer * html_renderer, const Param & param);
    // Return a fallback bg renderer according to param.bg_format.
    // Currently only svg bg format might need a bitmap fallback.
    static std::unique_ptr<BackgroundRenderer> getFallbackBackgroundRenderer(HTMLRenderer * html_renderer, const Param & param);

    BackgroundRenderer() {}
    virtual ~BackgroundRenderer() {}

    virtual void init(PDFDoc * doc) = 0;
    //return true on success, false otherwise (e.g. need a fallback)
    virtual bool render_page(PDFDoc * doc, int pageno) = 0;
    virtual void embed_image(int pageno) = 0;

    // for proof output
protected:
    void proof_begin_text_object(GfxState * state, OutputDev * dev);
    void proof_begin_string(GfxState * state, OutputDev * dev);
    void proof_end_text_object(GfxState * state, OutputDev * dev);
    void proof_update_render(GfxState * state, OutputDev * dev);
private:
    std::unique_ptr<GfxState> proof_state;
};

} // namespace pdf2htmlEX

#endif //BACKGROUND_RENDERER_H__
