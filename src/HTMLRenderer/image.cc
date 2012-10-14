/*
 * image.cc
 *
 * Handling images
 *
 * by WangLu
 * 2012.08.14
 */

#include "HTMLRenderer.h"
#include "namespace.h"

namespace pdf2htmlEX {

void HTMLRenderer::drawImage(GfxState * state, Object * ref, Stream * str, int width, int height, GfxImageColorMap * colorMap, GBool interpolate, int *maskColors, GBool inlineImg)
{
    return BackgroundRenderer::drawImage(state,ref,str,width,height,colorMap,interpolate,maskColors,inlineImg);
}

} // namespace pdf2htmlEX
