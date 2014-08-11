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
    std::ostringstream derp;

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
                << std::to_string(font_size) << "px;\" class=\"text_input\" />" << endl;
        }

        if(w->getType() == formButton)
        {
            out << "<div id=\"cb-" << std::to_string(pageNum) << "-" 
                << std::to_string(i) << "\"" 
                << " style=\"opacity:0.0; position: absolute; left: " << std::to_string(x1) << 
                "px; bottom: " << std::to_string(y1) << "px;" <<
                "width: " << std::to_string(width) << "px; height: " << std::to_string(height) << 
                "px;  font-size: 20px; \" class=\"checkbox-" <<
                std::to_string(pageNum) << "\">X</div>" << endl;

        }
    }

    //output, at the end, the necessary css
    if(num > 0) {
        //this is usable by the whole document and as such should be in dump_css
        out << "<style>" <<
            ".text_input {" <<
            "border: none; " << 
            "background-color: rgba(255, 255, 255, 0.0);" <<
            "}" << endl <<
            ".checkbox-" << std::to_string(pageNum) << ":hover {" <<
            "cursor: pointer;" <<
            "}" <<
            "</style>" << endl;
        
        //this is currently page specific
        out << "<script type=\"text/javascript\">" << endl << 
            "var checkboxes = document.getElementsByClassName(\"checkbox-" <<
            std::to_string(pageNum) << "\");" << endl <<
            "var c = checkboxes.item(0);" << endl <<
            "console.log(c);" << endl <<
            "for(var i = 0; i < checkboxes.length; i++) {" << endl <<
            "var c = checkboxes[i];" << endl <<
            "c.addEventListener('click', function() {" << endl <<
            "if(this.style.opacity == 1)" << endl << 
            "this.style.opacity = 0;" << endl <<
            "else" << endl << 
            "this.style.opacity = 1;" << endl <<
            "});" << endl <<
            "}" << endl <<
            "</script>" << endl;
        
    }
}

}
