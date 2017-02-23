/*
 * link.cc
 *
 * Handling links
 *
 * by WangLu
 * 2012.09.25
 */

#include <iostream>
#include <sstream>
#include <algorithm>

#include <Link.h>

#include "HTMLRenderer.h"
#include "util/namespace.h"
#include "util/math.h"
#include "util/misc.h"
#include "util/encoding.h"
#include "util/css_const.h"

namespace pdf2htmlEX {
   
using std::ostringstream;
using std::min;
using std::max;
using std::cerr;
using std::endl;

/*
 * The detailed rectangle area of the link destination
 * Will be parsed and performed by Javascript
 * The string will be put into a HTML attribute, surrounded by single quotes
 * So pay attention to the characters used here
 */
static string get_linkdest_detail_str(LinkDest * dest, Catalog * catalog, int & pageno)
{
    pageno = 0;
    if(dest->isPageRef())
    {
        auto pageref = dest->getPageRef();
        pageno = catalog->findPage(pageref.num, pageref.gen);
    }
    else
    {
        pageno = dest->getPageNum();
    }

    if(pageno <= 0)
    {
        return "";
    }

    ostringstream sout;
    // dec
    sout << "[" << pageno;

    switch(dest->getKind())
    {
        case destXYZ:
            {
                sout << ",\"XYZ\",";
                if(dest->getChangeLeft())
                    sout << (dest->getLeft());
                else
                    sout << "null";
                sout << ",";
                if(dest->getChangeTop())
                    sout << (dest->getTop());
                else
                    sout << "null";
                sout << ",";
                if(dest->getChangeZoom())
                    sout << (dest->getZoom());
                else
                    sout << "null";
            }
            break;
        case destFit:
            sout << ",\"Fit\"";
            break;
        case destFitH:
            sout << ",\"FitH\",";
            if(dest->getChangeTop())
                sout << (dest->getTop());
            else
                sout << "null";
            break;
        case destFitV:
            sout << ",\"FitV\",";
            if(dest->getChangeLeft())
                sout << (dest->getLeft());
            else
                sout << "null";
            break;
        case destFitR:
            sout << ",\"FitR\","
                << (dest->getLeft()) << ","
                << (dest->getBottom()) << ","
                << (dest->getRight()) << ","
                << (dest->getTop());
            break;
        case destFitB:
            sout << ",\"FitB\"";
            break;
        case destFitBH:
            sout << ",\"FitBH\",";
            if(dest->getChangeTop())
                sout << (dest->getTop());
            else
                sout << "null";
            break;
        case destFitBV:
            sout << ",\"FitBV\",";
            if(dest->getChangeLeft())
                sout << (dest->getLeft());
            else
                sout << "null";
            break;
        default:
            break;
    }
    sout << "]";

    return sout.str();
}

string HTMLRenderer::get_linkaction_str(LinkAction * action, string & detail)
{
    string dest_str;
    detail = "";
    if(action)
    {
        auto kind = action->getKind();
        switch(kind)
        {
            case actionGoTo:
                {
                    auto * real_action = dynamic_cast<LinkGoTo*>(action);
                    LinkDest * dest = nullptr;
                    if(auto _ = real_action->getDest())
                        dest = _->copy();
                    else if (auto _ = real_action->getNamedDest())
                        dest = cur_catalog->findDest(_);
                    if(dest)
                    {
                        int pageno = 0;
                        detail = get_linkdest_detail_str(dest, cur_catalog, pageno);
                        if(pageno > 0)
                        {
                            dest_str = (char*)str_fmt("#%s%x", CSS::PAGE_FRAME_CN, pageno);
                        }
                        delete dest;
                    }
                }
                break;
            case actionGoToR:
                {
                    cerr << "TODO: actionGoToR is not implemented." << endl;
                }
                break;
            case actionURI:
                {
                    auto * real_action = dynamic_cast<LinkURI*>(action);
                    assert(real_action != nullptr);
                    dest_str = real_action->getURI()->getCString();
                }
                break;
            case actionLaunch:
                {
                    cerr << "TODO: actionLaunch is not implemented." << endl;
                }
                break;
            default:
                cerr << "Warning: unknown annotation type: " << kind << endl;
                break;
        }
    }

    return dest_str;
}
    
/*
 * Based on pdftohtml from poppler
 * TODO: share rectangle draw with css-draw
 */
void HTMLRenderer::processLink(AnnotLink * al)
{
    string dest_detail_str;
    string dest_str = get_linkaction_str(al->getAction(), dest_detail_str);

    if(!dest_str.empty())
    {
        (*f_curpage) << "<a class=\"" << CSS::LINK_CN << "\" href=\"";
        writeAttribute((*f_curpage), dest_str);
        (*f_curpage) << "\"";

        if(!dest_detail_str.empty())
            (*f_curpage) << " data-dest-detail='" << dest_detail_str << "'";

        (*f_curpage) << ">";
    }

    (*f_curpage) << "<div class=\"" << CSS::CSS_DRAW_CN << ' ' << CSS::TRANSFORM_MATRIX_CN
        << all_manager.transform_matrix.install(default_ctm)
        << "\" style=\"";

    double x,y,w,h;
    double x1, y1, x2, y2;
    al->getRect(&x1, &y1, &x2, &y2);
    x = min<double>(x1, x2);
    y = min<double>(y1, y2);
    w = max<double>(x1, x2) - x;
    h = max<double>(y1, y2) - y;
    
    double border_width = 0; 
    double border_top_bottom_width = 0;
    double border_left_right_width = 0;
    auto * border = al->getBorder();
    if(border)
    {
        border_width = border->getWidth();
        if(border_width > 0)
        {
            {
                css_fix_rectangle_border_width(x1, y1, x2, y2, border_width, 
                        x, y, w, h,
                        border_top_bottom_width, border_left_right_width);

                if(std::abs(border_top_bottom_width - border_left_right_width) < EPS)
                    (*f_curpage) << "border-width:" << round(border_top_bottom_width) << "px;";
                else
                    (*f_curpage) << "border-width:" << round(border_top_bottom_width) << "px " << round(border_left_right_width) << "px;";
            }
            auto style = border->getStyle();
            switch(style)
            {
                case AnnotBorder::borderSolid:
                    (*f_curpage) << "border-style:solid;";
                    break;
                case AnnotBorder::borderDashed:
                    (*f_curpage) << "border-style:dashed;";
                    break;
                case AnnotBorder::borderBeveled:
                    (*f_curpage) << "border-style:outset;";
                    break;
                case AnnotBorder::borderInset:
                    (*f_curpage) << "border-style:inset;";
                    break;
                case AnnotBorder::borderUnderlined:
                    (*f_curpage) << "border-style:none;border-bottom-style:solid;";
                    break;
                default:
                    cerr << "Warning:Unknown annotation border style: " << style << endl;
                    (*f_curpage) << "border-style:solid;";
            }


            auto color = al->getColor();
            double r,g,b;
            if(color && (color->getSpace() == AnnotColor::colorRGB))
            {
                const double * v = color->getValues();
                r = v[0];
                g = v[1];
                b = v[2];
            }
            else
            {
                r = g = b = 0;
            }

            (*f_curpage) << "border-color:rgb("
                << dec << (int)dblToByte(r) << "," << (int)dblToByte(g) << "," << (int)dblToByte(b) << hex
                << ");";
        }
        else
        {
            (*f_curpage) << "border-style:none;";
        }
    }
    else
    {
        (*f_curpage) << "border-style:none;";
    }

    tm_transform(default_ctm, x, y);

    (*f_curpage) << "position:absolute;"
        << "left:" << round(x) << "px;"
        << "bottom:" << round(y) << "px;"
        << "width:" << round(w) << "px;"
        << "height:" << round(h) << "px;";

    // fix for IE
    (*f_curpage) << "background-color:rgba(255,255,255,0.000001);";

    (*f_curpage) << "\"></div>";

    if(dest_str != "")
    {
        (*f_curpage) << "</a>";
    }
}

}// namespace pdf2htmlEX
