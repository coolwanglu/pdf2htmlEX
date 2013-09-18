/*
 * Background renderer
 * Render all those things not supported as Image
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

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
    if(format == "png")
    {
        return new SplashBackgroundRenderer(html_renderer, param);
    }
    else if (format == "svg")
    {
#if ENABLE_SVG
        return new CairoBackgroundRenderer(html_renderer, param);
#else
        return nullptr;
#endif 
    }
    else
    {
        return nullptr;
    }
}

} // namespace pdf2htmlEX
