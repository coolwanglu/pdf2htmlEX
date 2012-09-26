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

#include <HTMLRenderer.h>
#include <Link.h>

#include "namespace.h"

namespace pdf2htmlEX {
   
using std::ostringstream;
using std::min;
using std::max;

static void _transform(const double * ctm, double & x, double & y)
{
    double xx = x, yy = y;
    x = ctm[0] * xx + ctm[2] * yy + ctm[4];
    y = ctm[1] * xx + ctm[3] * yy + ctm[5];
}

static void _get_transformed_rect(AnnotLink * link, const double * ctm, double & x1, double & y1, double & x2, double & y2)
{
    double _x1, _x2, _y1, _y2;
    link->getRect(&_x1, &_y1, &_x2, &_y2);

    _transform(ctm, _x1, _y1);
    _transform(ctm, _x2, _y2);

    x1 = min(_x1, _x2);
    x2 = max(_x1, _x2);
    y1 = min(_y1, _y2);
    y2 = max(_y1, _y2);
}

/*
 * In PDF, edges of the rectangle are in the middle of the borders
 * In HTML, edges are completely outside the rectangle
 */
static void _fix_border_width(double & x1, double & y1, double & x2, double & y2, 
        double border_width, double & border_top_bottom_width, double & border_left_right_width)
{
    double w = x2 - x1;
    if(w > border_width)
    {
        x1 += border_width / 2;
        x2 -= border_width / 2;
        border_left_right_width = border_width;
    }
    else
    {
        x1 += w / 2;
        x2 -= w / 2;
        border_left_right_width = border_width + w/2;
    }
    double h = y2 - y1;
    if(h > border_width)
    {
        y1 += border_width / 2;
        y2 -= border_width / 2;
        border_top_bottom_width = border_width;
    }
    else
    {
        y1 += h / 2;
        y2 -= h / 2;
        border_top_bottom_width = border_width + h/2;
    }
}

/*
 * The detailed rectangle area of the link destination
 * Will be parsed and performed by Javascript
 */
static string get_dest_detail_str(int pageno, LinkDest * dest)
{
    ostringstream sout;
    // dec
    sout << "[" << pageno;

    if(dest)
    {
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
    }
    sout << "]";

    return sout.str();
}
    
/*
 * Based on pdftohtml from poppler
 * TODO: CSS for link rectangles
 */
void HTMLRenderer::processLink(AnnotLink * al)
{
    std::string dest_str, dest_detail_str;
    auto action = al->getAction();
    if(action)
    {
        auto kind = action->getKind();
        switch(kind)
        {
            case actionGoTo:
                {
                    auto catalog = cur_doc->getCatalog();
                    auto * real_action = dynamic_cast<LinkGoTo*>(action);
                    LinkDest * dest = nullptr;
                    if(auto _ = real_action->getDest())
                        dest = _->copy();
                    else if (auto _ = real_action->getNamedDest())
                        dest = catalog->findDest(_);
                    if(dest)
                    {
                        int pageno = 0;
                        if(dest->isPageRef())
                        {
                            auto pageref = dest->getPageRef();
                            pageno = catalog->findPage(pageref.num, pageref.gen);
                        }
                        else
                        {
                            pageno = dest->getPageNum();
                        }

                        if(pageno > 0)
                        {
                            dest_str = (char*)str_fmt("#p%x", pageno);
                            dest_detail_str = get_dest_detail_str(pageno, dest);
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

    if(dest_str != "")
    {
        html_fout << "<a class=\"a\" href=\"" << dest_str << "\"";

        if(dest_detail_str != "")
            html_fout << " data-dest-detail='" << dest_detail_str << "'";

        html_fout << ">";
    }

    html_fout << "<div style=\"";

    double x1, y1, x2, y2;
    _get_transformed_rect(al, default_ctm, x1, y1, x2, y2);
    
    double border_width = 0; 
    double border_top_bottom_width = 0;
    double border_left_right_width = 0;
    auto * border = al->getBorder();
    if(border)
    {
        border_width = border->getWidth() * zoom_factor();
        if(border_width > 0)
        {
            {
                _fix_border_width(x1, y1, x2, y1, 
                        border_width, border_top_bottom_width, border_left_right_width);
                if(abs(border_top_bottom_width - border_left_right_width) < EPS)
                    html_fout << "border-width:" << _round(border_top_bottom_width) << "px;";
                else
                    html_fout << "border-width:" << _round(border_top_bottom_width) << "px " << _round(border_left_right_width) << "px;";
            }
            auto style = border->getStyle();
            switch(style)
            {
                case AnnotBorder::borderSolid:
                    html_fout << "border-style:solid;";
                    break;
                case AnnotBorder::borderDashed:
                    html_fout << "border-style:dashed;";
                    break;
                case AnnotBorder::borderBeveled:
                    html_fout << "border-style:outset;";
                    break;
                case AnnotBorder::borderInset:
                    html_fout << "border-style:inset;";
                    break;
                case AnnotBorder::borderUnderlined:
                    html_fout << "border-style:none;border-bottom-style:solid;";
                    break;
                default:
                    cerr << "Warning:Unknown annotation border style: " << style << endl;
                    html_fout << "border-style:solid;";
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

            html_fout << "border-color:rgb("
                << dec << (int)dblToByte(r) << "," << (int)dblToByte(g) << "," << (int)dblToByte(b) << hex
                << ");";
        }
        else
        {
            html_fout << "border-style:none;";
        }
    }
    else
    {
        html_fout << "border-style:none;";
    }


    html_fout << "position:absolute;"
        << "left:" << _round(x1- border_left_right_width) << "px;"
        << "bottom:" << _round(y1 - border_top_bottom_width) << "px;"
        << "width:" << _round(x2-x1) << "px;"
        << "height:" << _round(y2-y1) << "px;";

    // fix for IE
    html_fout << "background-color:rgba(255,255,255,0.000001);";

    html_fout << "\"></div>";

    if(dest_str != "")
    {
        html_fout << "</a>";
    }
}

}// namespace pdf2htmlEX
