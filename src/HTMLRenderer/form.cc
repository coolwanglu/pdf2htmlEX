/*
 * form.cc
 *
 * Handling Forms
 *
 * by Simon Chenard
 * 2014.07.25
 */

#include <iostream>
#include <sstream>
#include <string>

#include "HTMLRenderer.h"
#include "util/namespace.h"
#include "util/misc.h"

namespace pdf2htmlEX {
   
using std::ostream;
using std::cerr;

void HTMLRenderer::process_form(ostream & out)
{
    FormPageWidgets * widgets = cur_catalog->getPage(pageNum)->getFormWidgets();
    int num = widgets->getNumWidgets();

    for(int i = 0; i < num; i++)
    {
        FormWidget * w = widgets->getWidget(i);
        double x1, y1, x2, y2;
        int width, height, font_size;

        w->getRect(&x1, &y1, &x2, &y2);
        x1 = x1 * param.zoom;
        x2 = x2 * param.zoom;
        y1 = y1 * param.zoom;
        y2 = y2 * param.zoom;

        width = x2 - x1;
        height = y2 - y1;
        
        if(w->getType() == formText)
        {
            font_size = height / 2;

            out << "<input id=\"text-" << std::to_string(pageNum) << "-" 
                << std::to_string(i) << "\" type=\"text\" value=\"\""
                << " style=\"position: absolute; left: " << std::to_string(x1) << 
                "px; bottom: " << std::to_string(y1) << "px;" <<
                "width: " << std::to_string(width) << "px; height: " << std::to_string(height) << 
                "px; line-height: " << std::to_string(height) << "px; font-size: " 
                << std::to_string(font_size) << "px;\" class=\"" << CSS::INPUT_TEXT_CN << "\" />" << endl;
        }

        if(w->getType() == formButton)
        {
            width += 3;
            height += 3;

            out << "<div id=\"cb-" << std::to_string(pageNum) << "-" 
                << std::to_string(i) << "\"" 
                << " style=\"position: absolute; left: " << std::to_string(x1) << 
                "px; bottom: " << std::to_string(y1) << "px;" <<
                "width: " << std::to_string(width) << "px; height: " << std::to_string(height) << 
                "px; \" class=\"" << CSS::INPUT_RADIO_CN << "\"></div>" << endl;

        }
    }
}

}
