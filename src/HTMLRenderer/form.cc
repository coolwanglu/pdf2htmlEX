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

using namespace std;

namespace pdf2htmlEX {
   

void HTMLRenderer::process_form(ofstream & out)
{
    FormPageWidgets * widgets = cur_catalog->getPage(pageNum)->getFormWidgets();
    int num = widgets->getNumWidgets();

    cerr << "number of widgets: " << num << endl;

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
        double font_size = height / 2;           
           
		tm_transform(default_ctm, x1, y1);
        
        if(w->getType() == formText)
        {
	    cerr << "Found a text box" << endl;
            FormField *f = w->getField();
            Object *o = f->getObj();

            // the following line throws if the result of lookup doesnt return a string. 
            Object *lo = o->getDict()->lookup((char *)"T", o);

            if (lo->getType() == objNull) {
                // check the parent
                Object *wObj = w->getObj();
                cerr << "lo is obj null, so looking for parent" << endl;
                Dict *wDict = wObj->getDict();
                cerr << "Fetched parent dictionary" << endl;
                Object *parent = wDict->lookup((char *)"Parent", wObj);
                if (parent->getType() == objDict) {
		   cerr << "Fetching from parent dictionary" << endl;
                   lo = parent->getDict()->lookup((char *)"T", parent);
                }
            }
            
            if (lo->getType() == objString) { 
                char *name = lo->getString()->getCString();
                cerr << "Writing text box for " << name << endl;
                out << "<div id=\"text-" << pageNum << "-" << i
                    << "\" form-field=\"" << name
                    << "\" class=\"" << CSS::INPUT_TEXT_CN 
                    << "\" style=\"position: absolute; font-family:arial; left: " << x1 
                    << "px; bottom: " << y1 << "px;" 
                    << " width: " << width << "px; height: " << std::to_string(height) 
                    << "px; line-height: " << std::to_string(height) << "px; font-size: " 
                    << font_size << "px;\" ></div>" << endl;
            } else if (lo->getType() == objNull) {
		//Object *parentText = o->getDict()->lookup((char *)"Parent", o);

		cerr << "Found something else..." << o->getType() << endl; 
            }
        } 
        else if (w->getType() == formButton)
        {
            cerr << "Found a button" << endl;
            FormWidgetButton *b = dynamic_cast<FormWidgetButton*>(w);

            if (b->getButtonType() == formButtonCheck) {
                // grab the export value of the form button type....
                FormField *f = w->getField();
                Object *o = f->getObj();

                if (o->getType() == objDict) { 
                    cerr << "Is Dictionary" << endl;
                    cerr << o->getType() << endl;

                    //Object *widgeto = b->getObj();

                    Dict *od = o->getDict();

                    
                    // the following line throws if the result of lookup doesnt return a string. 
                    Object *formfielddescription = od->lookup((char *)"Parent", o);

                    cerr << "didnt throw" << endl;

                    if (formfielddescription->getType() == objDict) { 

                        Dict *formfielddictionary = formfielddescription->getDict();

                        Object *lo = formfielddictionary->lookup((char *)"T", formfielddescription);
        
                        if (lo->getType() == objString) { 
                            char *fieldname = lo->getString()->getCString();

                            cerr << "first branch" << endl;
                            cerr << fieldname << endl;

                            char *name = b->getOnStr();

                            cerr << name << endl;

                            out << "<div id=\"text-" << pageNum << "-" << i
                                << "\" form-field=\"" << fieldname
                                << "\" export-value=\"" << name
                                << "\" class=\"" << CSS::INPUT_TEXT_CN 
                                << "\" style=\"position: absolute; font -family:arial; left: " << x1 
                                << "px; bottom: " << y1 << "px;" 
                                << " width: " << width << "px; height: " << std::to_string(height) 
                                << "px; line-height: " << std::to_string(height) << "px; font-size: " 
                                << font_size << "px;\" ></div>" << endl;    
                        }
                    } else if (od->lookup((char *)"T", o)->getType() == objString) { 
                        char *fieldname = od->lookup((char *)"T", o)->getString()->getCString();
                        char *name = b->getOnStr();

                        cerr << "second branch" << endl;
                        cerr << fieldname << endl;
                        cerr << name << endl;

                        out << "<div id=\"text-" << pageNum << "-" << i
                            << "\" form-field=\"" << fieldname
                            << "\" export-value=\"" << name
                            << "\" class=\"" << CSS::INPUT_TEXT_CN 
                            << "\" style=\"position: absolute; font -family:arial; left: " << x1 
                            << "px; bottom: " << y1 << "px;" 
                            << " width: " << width << "px; height: " << std::to_string(height) 
                            << "px; line-height: " << std::to_string(height) << "px; font-size: " 
                            << font_size << "px;\" ></div>" << endl;
                    }
                } 
            }
        }
        else 
        {
            cerr << "Unsupported form field detected" << endl;
        }
    }
    cerr << flush;
}

}
