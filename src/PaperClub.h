/*
 * HTMLRenderer for PaperClub
 *
 * by WangLu
 * 2012.08.13
 */

#ifndef PAPERCLUB_H__
#define PAPERCLUB_H__

#include "HTMLRenderer.h"
#include "namespace.h"

class PC_HTMLRenderer : public HTMLRenderer
{
    public:
        PC_HTMLRenderer (const Param * param)
           : HTMLRenderer(param)
        { } 

        virtual ~PC_HTMLRenderer() { }

        virtual void write_html_head() { }
        virtual void write_html_tail() { }

        virtual void startPage(int pageNum, GfxState *state) 
        {
            html_fout.close();
            html_fout.open(dest_dir / (format("%|1$x|.page")%pageNum).str(), ofstream::binary);

            HTMLRenderer::startPage(pageNum, state);
        }

        virtual void endPage()
        {
            HTMLRenderer::endPage();
            html_fout.close();
        }
};


#endif //PAPERCLUB_H__
