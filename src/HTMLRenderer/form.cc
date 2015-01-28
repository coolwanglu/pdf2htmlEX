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
   
using std::ofstream;
using std::cerr;

void HTMLRenderer::process_form(ofstream & out)
{
    FormPageWidgets * widgets = cur_catalog->getPage(pageNum)->getFormWidgets();
    int num = widgets->getNumWidgets();

    for(int i = 0; i < num; i++)
    {
        FormWidget * w = widgets->getWidget(i);
        double x1, y1, x2, y2;

        w->getRect(&x1, &y1, &x2, &y2);
        x1 = x1 * param.zoom;
        x2 = x2 * param.zoom;
        y1 = y1 * param.zoom;
        y2 = y2 * param.zoom;

        double width = x2 - x1;
        double height = y2 - y1;
        
        if(w->getType() == formText)
        {
            double font_size = height / 2;

            out << "<input id=\"text-" << pageNum << "-" << i 
                << "\" class=\"" << CSS::INPUT_TEXT_CN 
                << "\" type=\"text\" value=\"\""
                << " style=\"position: absolute; left: " << x1 
                << "px; bottom: " << y1 << "px;" 
                << " width: " << width << "px; height: " << std::to_string(height) 
                << "px; line-height: " << std::to_string(height) << "px; font-size: " 
                << font_size << "px;\" />" << endl;
        } 
        else if(w->getType() == formButton)
        {
            //Ideally would check w->getButtonType()
            //for more specific rendering
            width += 3;
            height += 3;

            out << "<div id=\"cb-" << pageNum << "-" << i 
                << "\" class=\"" << CSS::INPUT_RADIO_CN 
                << "\" style=\"position: absolute; left: " << x1 
                << "px; bottom: " << y1 << "px;" 
                << " width: " << width << "px; height: " 
                << std::to_string(height) << "px; background-size: cover;\" ></div>" << endl;
        }
        else 
        {
            cerr << "Unsupported form field detected" << endl;
        }
    }
}

}
