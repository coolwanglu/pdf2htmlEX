/*
 * Background renderer
 * Render all those things not supported as Image
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#include <poppler-config.h>

#include "HTMLRenderer/HTMLRenderer.h"
#include "Param.h"

#include "BackgroundRenderer.h"
#include "SplashBackgroundRenderer.h"
#if ENABLE_SVG
#include "CairoBackgroundRenderer.h"
#endif

namespace pdf2htmlEX {

BackgroundRenderer * BackgroundRenderer::getBackgroundRenderer(const std::string & format, HTMLRenderer * html_renderer, const Param & param)
{
#ifdef ENABLE_LIBPNG
    if(format == "png")
    {
        return new SplashBackgroundRenderer(format, html_renderer, param);
    }
#endif
#ifdef ENABLE_LIBJPEG
    if(format == "jpg")
    {
        return new SplashBackgroundRenderer(format, html_renderer, param);
    }
#endif
#if ENABLE_SVG
    if (format == "svg")
    {
        return new CairoBackgroundRenderer(html_renderer, param);
    }
#endif

    return nullptr;
}

BackgroundRenderer * BackgroundRenderer::getFallbackBackgroundRenderer(HTMLRenderer * html_renderer, const Param & param)
{
    if (param.bg_format == "svg" && param.svg_node_count_limit >= 0)
        return new SplashBackgroundRenderer("", html_renderer, param);
    return nullptr;
}

BackgroundRenderer::~BackgroundRenderer()
{
    //delete proof_state
    if (proof_state)
        delete proof_state;
}

void BackgroundRenderer::proof_begin_string(GfxState *state)
{
    auto dev = (OutputDev*) dynamic_cast<SplashBackgroundRenderer*>(this);
    if (!dev)
        dev = (OutputDev*) dynamic_cast<CairoBackgroundRenderer*>(this);

    if (!proof_state)
    {
        PDFRectangle rect(0, 0, state->getPageWidth(), state->getPageHeight());
        proof_state = new GfxState(state->getHDPI(), state->getVDPI(), &rect, state->getRotate(), dev->upsideDown());
        proof_state->setFillColorSpace(new GfxDeviceRGBColorSpace());
        proof_state->setStrokeColorSpace(new GfxDeviceRGBColorSpace());
    }

    Color fc, sc, red(1, 0, 0), green(0, 1, 0), blue(0, 0, 1), yellow(1, 1, 0);
    state->getFillRGB(&fc.rgb);
    state->getStrokeRGB(&sc.rgb);

    fc = fc.distance(red) >  0.4 ?  red : green;
    sc = sc.distance(blue) >  0.4 ? blue : yellow;

    if (state->getFillColorSpace()->getMode() != csDeviceRGB)
        dev->updateFillColorSpace(proof_state);
    if (state->getStrokeColorSpace()->getMode() != csDeviceRGB)
        dev->updateStrokeColorSpace(proof_state);
    GfxColor gfc, gsc;
    fc.get_gfx_color(gfc);
    sc.get_gfx_color(gsc);
    proof_state->setFillColor(&gfc);
    proof_state->setStrokeColor(&gsc);
    dev->updateFillColor(proof_state);
    dev->updateStrokeColor(proof_state);
}

void BackgroundRenderer::proof_end_text_object(GfxState *state)
{
    auto dev = (OutputDev*) dynamic_cast<SplashBackgroundRenderer*>(this);
    if (!dev)
        dev = (OutputDev*) dynamic_cast<CairoBackgroundRenderer*>(this);

    dev->updateFillColorSpace(state);
    dev->updateStrokeColorSpace(state);
    dev->updateFillColor(state);
    dev->updateStrokeColor(state);
}

} // namespace pdf2htmlEX
