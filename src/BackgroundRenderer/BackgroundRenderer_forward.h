/*
 * Background renderer
 * Render all those things not supported as Image
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */


/*
 * This file includes forward declarations since HTMLRenderer and BackgroundRendere have cross references
 */

#ifndef BACKGROUND_RENDERER_FORWARD_H__
#define BACKGROUND_RENDERER_FORWARD_H__

#include "pdf2htmlEX-config.h"

namespace pdf2htmlEX {
#if ENABLE_SVG
    class CairoBackgroundRenderer;
    typedef CairoBackgroundRenderer BackgroundRenderer;
#else
    class SplashBackgroundRenderer;
    typedef SplashBackgroundRenderer BackgroundRenderer;
#endif // ENABLE_SVG
}


#endif //BACKGROUND_RENDERER_FORWARD_H__
