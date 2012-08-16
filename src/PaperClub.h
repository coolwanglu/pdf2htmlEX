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

static GooString* getInfoDate(Dict *infoDict, const char *key) {
    Object obj;
    char *s;
    int year, mon, day, hour, min, sec, tz_hour, tz_minute;
    char tz;
    struct tm tmStruct;
    GooString *result = NULL;
    char buf[256];

    if (infoDict->lookup(key, &obj)->isString()) {
        s = obj.getString()->getCString();
        // TODO do something with the timezone info
        if ( parseDateString( s, &year, &mon, &day, &hour, &min, &sec, &tz, &tz_hour, &tz_minute ) ) {
            tmStruct.tm_year = year - 1900;
            tmStruct.tm_mon = mon - 1;
            tmStruct.tm_mday = day;
            tmStruct.tm_hour = hour;
            tmStruct.tm_min = min;
            tmStruct.tm_sec = sec;
            tmStruct.tm_wday = -1;
            tmStruct.tm_yday = -1;
            tmStruct.tm_isdst = -1;
            mktime(&tmStruct); // compute the tm_wday and tm_yday fields
            if (strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S+00:00", &tmStruct)) {
                result = new GooString(buf);
            } else {
                result = new GooString(s);
            }
        } else {
            result = new GooString(s);
        }
    }
    obj.free();
    return result;
}

class PC_HTMLRenderer : public HTMLRenderer
{
    public:
        PC_HTMLRenderer (const Param * param)
           : HTMLRenderer(param)
           , max_font_size (0)
        {} 

        virtual ~PC_HTMLRenderer() 
        { 
            if(param->only_metadata) {
                cout  << "{"
                      << "\"title\":\"" << cur_title.str() << "\","
                      << "\"num_pages\":" << num_pages << ","
                      << "\"modified_date\":\"" << modified_date << "\""
                      << "}" << endl;
            }
        }

        virtual void pre_process() 
        {
            if(!param->only_metadata) {
                allcss_fout.open(dest_dir / CSS_FILENAME, ofstream::binary);
                allcss_fout << ifstream(PDF2HTMLEX_LIB_PATH / CSS_FILENAME, ifstream::binary).rdbuf();
            }
        }

        virtual void process(PDFDoc * doc) 
        {
            if(param->only_metadata) {
                xref = doc->getXRef();

                num_pages = doc->getNumPages();
                // get meta info
                Object info;
                doc->getDocInfo(&info);
                if (info.isDict()) {
                   GooString* date = getInfoDate(info.getDict(), "ModDate");
                   if( !date )
                      date = getInfoDate(info.getDict(), "CreationDate");
                   modified_date = std::string(date->getCString(), date->getLength());
                }
                info.free();
                 
                pre_process();

                doc->displayPage(this, 1, param->zoom * DEFAULT_DPI, param->zoom * DEFAULT_DPI,
                0, true, false, false,
                nullptr, nullptr, nullptr, nullptr);

                post_process();
            }
            else {
                HTMLRenderer::process(doc);
            }
        }


        virtual void post_process() { 
            if(!param->only_metadata) {
                allcss_fout.close();
            }
        }

        virtual void startPage(int pageNum, GfxState *state) 
        {
            if(!param->only_metadata) {
                html_fout.open(dest_dir / (format("%|1$x|.page")%pageNum).str(), ofstream::binary);
            }
            HTMLRenderer::startPage(pageNum, state);
        }

        virtual void endPage()
        {
            HTMLRenderer::endPage();
            if(!param->only_metadata) {
                html_fout.close();
            }
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
                outputUnicodes2(cur_title, u, uLen);
                p += n;
                len -= n;
            } 

            tx = cur_tx;
            ty = cur_ty;
        }

        double max_font_size;
        double tx, ty;
        ostringstream cur_title;

  private:
        int num_pages;
        std::string modified_date;
};


#endif //PAPERCLUB_H__
