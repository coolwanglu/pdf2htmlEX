/*
 * image.cc
 *
 * Handling images
 *
 * by WangLu
 * 2012.08.14
 */

#if 0
// for gil bug
const int *int_p_NULL = nullptr;
#include <boost/gil/gil_all.hpp>
#include <boost/gil/extension/io/png_dynamic_io.hpp>
#endif

#include "HTMLRenderer.h"
#include "namespace.h"

//using namespace boost::gil;

void HTMLRenderer::drawImage(GfxState * state, Object * ref, Stream * str, int width, int height, GfxImageColorMap * colorMap, GBool interpolate, int *maskColors, GBool inlineImg)
{
    return OutputDev::drawImage(state,ref,str,width,height,colorMap,interpolate,maskColors,inlineImg);

#if 0
    if(maskColors)
        return;

    rgb8_image_t img(width, height);
    auto imgview = view(img);
    auto loc = imgview.xy_at(0,0);

    ImageStream * img_stream = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
    img_stream->reset();

    for(int i = 0; i < height; ++i)
    {
        auto p = img_stream->getLine();
        for(int j = 0; j < width; ++j)
        {
            GfxRGB rgb;
            colorMap->getRGB(p, &rgb);

            *loc = rgb8_pixel_t(colToByte(rgb.r), colToByte(rgb.g), colToByte(rgb.b));

            p += colorMap->getNumPixelComps();

            ++ loc.x();
        }

        loc = imgview.xy_at(0, i+1);
    }

    png_write_view((format("i%|1$x|.png")%image_count).str(), imgview);
    
    img_stream->close();
    delete img_stream;

    close_line();

    double ctm[6];
    memcpy(ctm, state->getCTM(), sizeof(ctm));
    ctm[4] = ctm[5] = 0.0;
    html_fout << format("<img class=\"i t%2%\" style=\"left:%3%px;bottom:%4%px;width:%5%px;height:%6%px;\" src=\"i%|1$x|.png\" />") % image_count % install_transform_matrix(ctm) % state->getCurX() % state->getCurY() % width % height << endl;


    ++ image_count;
#endif
}
