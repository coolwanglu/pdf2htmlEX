/*
 * HTMLRenderer for PaperClub
 *
 * by WangLu
 * 2012.08.13
 */

#ifndef PAPERCLUB_H__
#define PAPERCLUB_H__

#include <sstream>

#include "HTMLRenderer.h"
#include "HTMLRenderer/namespace.h"

using std::ostringstream;

class PC_HTMLRenderer : public HTMLRenderer
{
    public:
        PC_HTMLRenderer (const Param * param)
           : HTMLRenderer(param)
           , max_font_size (0)
        { } 

        virtual ~PC_HTMLRenderer() 
        { 
            cerr << "Title: " << cur_title.str() << endl;
        }

        virtual void pre_process() 
        {
            allcss_fout.open(dest_dir / CSS_FILENAME, ofstream::binary);
            allcss_fout << ifstream(PDF2HTMLEX_LIB_PATH / CSS_FILENAME, ifstream::binary).rdbuf();
        }

        virtual void post_process() { }

        virtual void startPage(int pageNum, GfxState *state) 
        {
            html_fout.open(dest_dir / (format("%|1$x|.page")%pageNum).str(), ofstream::binary);
            HTMLRenderer::startPage(pageNum, state);
        }

        virtual void endPage()
        {
            HTMLRenderer::endPage();
            html_fout.close();
        }

        virtual void drawString(GfxState * state, GooString * s)
        {
            //determine padding now
            string padding =  (_equal(cur_ty, ty) && (cur_tx < tx + EPS)) ? "" : " ";
            
            // wait for status update
            HTMLRenderer::drawString(state, s);

            if(pageNum != 1) return;

            if((state->getRender() & 3) == 3) return;
            
            auto font = state->getFont();
            if(font == nullptr) return;

            double fs = state->getTransformedFontSize();
            if(fs < max_font_size) return;
            if(fs > max_font_size)
            {
                max_font_size = fs;
                cur_title.str("");
            }
            else
            {
                cur_title << padding;
            }

            char *p = s->getCString();
            int len = s->getLength();
            
            CharCode code;
            Unicode * u;
            int uLen;
            double dx, dy, ox, oy;

            while(len > 0)
            {
                int n = font->getNextChar(p, len, &code, &u, &uLen, &dx, &dy, &ox, &oy);
                outputUnicodes(cur_title, u, uLen);
                p += n;
                len -= n;
            } 

            tx = cur_tx;
            ty = cur_ty;
        }

        double max_font_size;
        double tx, ty;
        ostringstream cur_title;
};


#endif //PAPERCLUB_H__
