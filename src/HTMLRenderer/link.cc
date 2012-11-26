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
 * TODO: share rectangle draw with css-draw
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

    double x1, y1, x2, y2;
    al->getRect(&x1, &y1, &x2, &y2);

	device.link( x1, y1, x2, y2, install_transform_matrix(default_ctm), default_ctm, dest_str, dest_detail_str, al->getBorder(), al->getColor() );
}

}// namespace pdf2htmlEX
