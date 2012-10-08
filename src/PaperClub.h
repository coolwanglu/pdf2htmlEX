/*
 * HTMLRenderer for PaperClub
 *
 * by WangLu
 * 2012.08.13
 */

#ifndef PAPERCLUB_H__
#define PAPERCLUB_H__

#include <iostream>
#include <sstream>

#include <DateInfo.h>

#include "HTMLRenderer.h"
#include "namespace.h"

using std::ostringstream;
using std::fixed;

using namespace pdf2htmlEX;

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

class SizedString
{
public:
    bool tm_like_id;
    double font_size;
    std::string str;
};

bool like_id_matrix(const double * tm)
{
    return tm[0] > EPS
        && std::abs(tm[1]) < EPS
        && std::abs(tm[2]) < EPS
        && tm[3] > EPS
        && std::abs(tm[0]-tm[3]) < EPS;
}

// compare font size only
// used when looking for title candidates
bool title_fs_cmp(const SizedString & ss1, const SizedString & ss2)
{
    return ss1.font_size > ss2.font_size;
}
    
// compare rotation, and length, besides font size
// used when sorting title candidates
bool title_cmp(const SizedString & ss1, const SizedString & ss2)
{
    int _1 = (ss1.str.size() >= 7) ? 1 : 0;
    int _2 = (ss2.str.size() >= 7) ? 1 : 0;
    if(_1 > _2) return true;
    if(_1 < _2) return false;
    _1 = ss1.tm_like_id ? 1 : 0;
    _2 = ss2.tm_like_id ? 1 : 0;
    if(_1 > _2) return true;
    if(_1 < _2) return false;

    return (ss1.font_size > ss2.font_size);
}

class PC_HTMLRenderer : public HTMLRenderer
{
    public:
        PC_HTMLRenderer (const Param * param)
           : HTMLRenderer(param)
           , tx(0)
           , ty(0)
           , font_size (0)
           , tm_id(-1)
           , fn_id(-1)
           , num_pages(0)
           , first_page_width(0)
           , first_page_height(0)
        {} 

        virtual ~PC_HTMLRenderer() 
        { 
            if(param->only_metadata) {
                cout  << "{"
                      << "\"title\":[";
                
                std::sort(title_candidates.begin(), title_candidates.end(), title_cmp);
                bool f = true;
                for(auto s : title_candidates)
                {
                    if(f) f = false;
                    else cout << ",";
                    cout << "\"" << s.str << "\"";
                }

                cout << "],"
                      << "\"num_pages\":" << num_pages << ","
                      << "\"modified_date\":\"" << modified_date << "\","
                      << "\"width\":" << first_page_width << ","
                      << "\"height\":" << first_page_height
                      << "}" << endl;
            }
        }

        virtual void pre_process() 
        {
            if(!param->only_metadata) {
                HTMLRenderer::pre_process();   
            }
        }

        virtual void process(PDFDoc * doc) 
        {
            if(param->only_metadata) {
                cur_doc = doc;
                xref = doc->getXRef();

                num_pages = doc->getNumPages();
                // get meta info
                Object info;
                doc->getDocInfo(&info);
                if (info.isDict()) {
                   GooString* date = getInfoDate(info.getDict(), "ModDate");
                   if( !date )
                      date = getInfoDate(info.getDict(), "CreationDate");
                   if( date )
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
                HTMLRenderer::post_process();
                // Touch a file to indicate processing is done
                ofstream touch_done_file(param->dest_dir + "/all.css.done", ofstream::binary);
            }
        }

        virtual void startPage(int pageNum, GfxState *state) 
        {
            if(param->only_metadata)
            {
                this->first_page_width = state->getPageWidth();
                this->first_page_height = state->getPageHeight();
            }

            HTMLRenderer::startPage(pageNum, state);
        }

        virtual void endPage()
        {
            HTMLRenderer::endPage();
            if(!param->only_metadata) {
                css_fout << "#p" << hex << (this->pageNum) << "{visibility: visible;}" << endl;
            }
        }

        virtual void drawString(GfxState * state, GooString * s)
        {
            //determine padding now
            double x,y;
            state->textTransform(cur_tx, cur_ty, &x, &y);
            string padding = (_equal(y, ty) && (x < tx + param->h_eps)) ? "" : " ";
            
            // wait for status update
            HTMLRenderer::drawString(state, s);

            state->textTransform(cur_tx, cur_ty, &x, &y);
            tx = x;
            ty = y;

            if(pageNum != 1) return;

            if((state->getRender() & 3) == 3) return;
            
            auto font = state->getFont();
            if(font == nullptr) return;

            double fs = state->getTransformedFontSize();
            if((std::abs(fs - font_size) > EPS) || (cur_ttm_id != tm_id) || (cur_font_info->id != fn_id))
            {
                SizedString ss;
                ss.tm_like_id = tm_map[tm_id];
                ss.font_size = font_size;
                ss.str = cur_title.str();
                title_candidates.push_back(ss);
                std::push_heap(title_candidates.begin(), title_candidates.end(), title_fs_cmp);
                while(title_candidates.size() > 7)
                {
                    std::pop_heap(title_candidates.begin(), title_candidates.end(), title_fs_cmp);
                    title_candidates.pop_back();
                }

                font_size = fs;
                tm_id = cur_ttm_id;
                fn_id = cur_font_info->id;
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
        }

        double tx, ty;
        double font_size;
        long long tm_id;
        long long fn_id;
        ostringstream cur_title;
        std::vector<SizedString> title_candidates;

  protected: 
        virtual const FontInfo * install_font(GfxFont * font) {
            if (param->only_metadata)
            {
                long long fn_id = (font == nullptr) ? 0 : hash_ref(font->getID());
                auto iter = font_name_map.find(fn_id);
                if(iter != font_name_map.end())
                    return &(iter->second);
                long long new_fn_id = font_name_map.size(); 
                auto cur_info_iter = font_name_map.insert(make_pair(fn_id, FontInfo({new_fn_id, true}))).first;
                return &(cur_info_iter->second);
            }
            else
                return HTMLRenderer::install_font(font); 
        }

        virtual long long install_transform_matrix(const double * tm)
        {
            long long r = HTMLRenderer::install_transform_matrix(tm);
            tm_map[r] = like_id_matrix(tm);
            return r;
        }


  private:
        int num_pages;
        std::string modified_date;
        int first_page_width;
        int first_page_height;
        std::unordered_map<long long, bool> tm_map;
};


#endif //PAPERCLUB_H__
