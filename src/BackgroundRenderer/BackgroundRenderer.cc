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

std::unique_ptr<BackgroundRenderer> BackgroundRenderer::getBackgroundRenderer(const std::string & format, HTMLRenderer * html_renderer, const Param & param)
{
#ifdef ENABLE_LIBPNG
    if(format == "png")
    {
        return std::unique_ptr<BackgroundRenderer>(new SplashBackgroundRenderer(format, html_renderer, param));
    }
#endif
#ifdef ENABLE_LIBJPEG
    if(format == "jpg")
    {
        return std::unique_ptr<BackgroundRenderer>(new SplashBackgroundRenderer(format, html_renderer, param));
    }
#endif
#if ENABLE_SVG
    if (format == "svg")
    {
        return std::unique_ptr<BackgroundRenderer>(new CairoBackgroundRenderer(html_renderer, param));
    }
#endif

    return nullptr;
}

std::unique_ptr<BackgroundRenderer> BackgroundRenderer::getFallbackBackgroundRenderer(HTMLRenderer * html_renderer, const Param & param)
{
    if (param.bg_format == "svg" && param.svg_node_count_limit >= 0)
        return std::unique_ptr<BackgroundRenderer>(new SplashBackgroundRenderer("", html_renderer, param));
    return nullptr;
}

void BackgroundRenderer::proof_begin_text_object(GfxState *state, OutputDev * dev)
{
    if (!proof_state)
    {
        PDFRectangle rect(0, 0, state->getPageWidth(), state->getPageHeight());
        proof_state.reset(new GfxState(state->getHDPI(), state->getVDPI(), &rect, state->getRotate(), dev->upsideDown()));
        proof_state->setFillColorSpace(new GfxDeviceRGBColorSpace());
        proof_state->setStrokeColorSpace(new GfxDeviceRGBColorSpace());
    }

    // Save original render mode in proof_state, and restore in proof_end_text_object()
    // This is due to poppler's OutputDev::updateRender() actually has no effect, we have to
    // modify state directly, see proof_begin_string().
    proof_state->setRender(state->getRender());
}

void BackgroundRenderer::proof_begin_string(GfxState *state, OutputDev * dev)
{
    int render = proof_state->getRender();
    if (render == 3) // hidden
        return;

    double lx = state->getFontSize() / 70, ly = lx;
    tm_transform(state->getTextMat(), lx, ly, true);
    proof_state->setLineWidth(sqrt(lx * lx + ly * ly));

    static const Color red(1, 0, 0), green(0, 1, 0), blue(0, 0, 1), yellow(1, 1, 0), white(1, 1, 1);
    Color fc, sc;
    const Color *pfc, *psc;
    state->getFillRGB(&fc.rgb);
    state->getStrokeRGB(&sc.rgb);

    if (render == 0 || render == 2) //has fill
        pfc = fc.distance(red) >  0.4 ? &red : &green;
    else
        pfc = &red;

    if (render == 1 || render == 2) // has stroke
        psc = sc.distance(blue) >  0.4 ?  &blue : &yellow;
    else if(render == 0) // fill only
        psc = &white;
    else
        psc = &blue;

    GfxColor gfc, gsc;
    pfc->get_gfx_color(gfc);
    psc->get_gfx_color(gsc);
    proof_state->setFillColor(&gfc);
    proof_state->setStrokeColor(&gsc);

    if (state->getFillColorSpace()->getMode() != csDeviceRGB)
        dev->updateFillColorSpace(proof_state.get());
    if (state->getStrokeColorSpace()->getMode() != csDeviceRGB)
        dev->updateStrokeColorSpace(proof_state.get());

    dev->updateLineWidth(proof_state.get());
    dev->updateFillColor(proof_state.get());
    dev->updateStrokeColor(proof_state.get());

    state->setRender(2); // fill & stroke
}

void BackgroundRenderer::proof_end_text_object(GfxState *state, OutputDev * dev)
{
    state->setRender(proof_state->getRender());
    dev->updateLineWidth(state);
    dev->updateFillColorSpace(state);
    dev->updateStrokeColorSpace(state);
    dev->updateFillColor(state);
    dev->updateStrokeColor(state);
}

void BackgroundRenderer::proof_update_render(GfxState *state, OutputDev * dev)
{
    // Save render mode in proof_state in cases it is changed inside a text object
    proof_state->setRender(state->getRender());
}

} // namespace pdf2htmlEX
