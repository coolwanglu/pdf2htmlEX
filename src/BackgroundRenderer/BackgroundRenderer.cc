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
    if (format == "png" || format == "jpg")
        return new SplashBackgroundRenderer(format, html_renderer, param);

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
    if (param.bg_format == "svg" && param.svg_nodes_limit > 0)
        return new SplashBackgroundRenderer("", html_renderer, param);
    return nullptr;
}

} // namespace pdf2htmlEX
