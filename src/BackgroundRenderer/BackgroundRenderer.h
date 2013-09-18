/*
 * Background renderer
 * Render all those things not supported as Image
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef BACKGROUND_RENDERER_H__
#define BACKGROUND_RENDERER_H__

#include "pdf2htmlEX-config.h"

#if ENABLE_SVG

#include "CairoBackgroundRenderer.h"

namespace pdf2htmlEX {
    typedef CairoBackgroundRenderer BackgroundRenderer;
}

#else

#include "SplashBackgroundRenderer.h" 

namespace pdf2htmlEX {
    typedef SplashBackgroundRenderer BackgroundRenderer;
}

#endif // ENABLE_SVG

#endif //BACKGROUND_RENDERER_H__
