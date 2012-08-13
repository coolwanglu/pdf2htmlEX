/*
 * HTMLRenderer for PaperClub
 *
 * by WangLu
 * 2012.08.13
 */

#ifndef PAPERCLUB_H__
#define PAPERCLUB_H__

#include "HTMLRenderer.h"

class PC_HTMLRenderer : public HTMLRenderer
{
    public:
        virtual ~PC_HTMLRenderer()
        {
        }

        virtual void startPage(int pageNum, GfxState *state) 
        {
            html_fout.close();
            html_fout.open((boost::format("%|1$x|.page")%pageNum).str().c_str(), ofstream::binary);

            HTMLRenderer::startPage(pageNum, state);
        }

        virtual void endPage()
        {
            HTMLRenderer::endPage();
            html_fout.close();
        }
};


#endif //PAPERCLUB_H__
