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
#include <algorithm>

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
        // Define font_unit with a default of px this will change to pt if there is a defined font in the pdf template
        std::string font_unit = "px";
		tm_transform(default_ctm, x1, y1);
        
        if(w->getType() == formText)
        {
	    cerr << "Found a text box" << endl;
            FormField *f = w->getField();
            Object *o = f->getObj();

            // the following line throws if the result of lookup doesnt return a string. 
            Object *lo = new Object();
            o->getDict()->lookup((char *)"T", lo);
            Object *ff = new Object();
            o->getDict()->lookup((char *)"Ff", ff);

            // DA is an null || string that represents a few things such font family, font size ect.
            // Ex. looks like '/Helv 6 Tf 0 g' where Helv is the font family
            // and 6 is the font size.
            Object *da = new Object();
            o->getDict()->lookup((char *)"DA", da);

            // Q is an null || int that represents the text alignmnet if it is null or 0 it means
            // left aligned which is default. 1 stands for center aligned and 2 stands for right aligned.
            Object *q = new Object();
            o->getDict()->lookup((char *)"Q", q);

            //Object *po = o->getDict()->lookup((char *)"T", o);
            cerr << "Success" << endl;
            //o = f->getObj(); 
            //Object *ffEntry = o->getDict()->lookup((char *)"T", o);
            int ffvalue = 0;
            int multiline_flag = 4096;
            if (ff->getType() == objInt) { 
            //    cerr << "FFlo stuff" << endl;
                ffvalue = ff->getInt();
                cerr << "Found fflo" << ffvalue << endl;
            }
            std::string classes = "";
            classes = classes + CSS::INPUT_TEXT_CN;

            if (ffvalue & multiline_flag) { 
                cerr << "This is a multiline field" << endl;
                classes = classes + " multiline";
            }

            if (lo->getType() == objNull) {
                // check the parent
                Object *wObj = w->getObj();
                cerr << "lo is obj null, so looking for parent" << endl;
                Dict *wDict = wObj->getDict();
                cerr << "Fetched parent dictionary" << endl;
                Object *parent = wDict->lookup((char *)"Parent", wObj);
                if (parent->getType() == objDict) {
		   cerr << "Fetching from parent dictionary" << endl;

                    // If lo was null this means that there is a parent with the
                    // missing info. The same is likely the case both da and q which
                    // live at the same level so we repeat the lookups if they are null.
                    if (da->getType() == objNull) {
                        parent->getDict()->lookup((char *)"DA", da);
                    }

                    if (q->getType() == objNull) {
                        parent->getDict()->lookup((char *)"Q", q);
                    }

                   lo = parent->getDict()->lookup((char *)"T", parent);
                }
            }

            // If we have a value for da we want to use it to set the font size
            // however, the font size is stored with other data so we must strip
            // it down to just font size. Ex. looks like '/Helv 6 Tf 0 g'
            // where Helv is the font family and 6 is the font size.
            if (da->getType() == objString) {
                // da is just a pointer so we need to output the value into a temporary variable.
                std::string davalue = da->getString()->getCString();
                // Removes everything from string that is not a digit or space
                // this will convert it from looking like '/Helv 6 Tf 0 g'
                // to looking something like '6 0'
                davalue.erase(
                    std::remove_if(davalue.begin(), davalue.end(), [](char ch) { return !std::isdigit(ch) && !std::isspace(ch);}),
                    davalue.end()
                );

                // Call stoi() passing the davalue this will grab the first number and stop at space character
                // this will convert it from something like '6 0' to '6'.
                font_size = std::stoi(davalue);
                // If da has a value then we will have a font size that needs to be pt not px
                font_unit = "pt";
            }

            // Default value for text_align is left
            std::string text_align = "left";
            // If we have a q value this means that we have an alignmnet and must set it 
            // based off of 1 for center and 2 for right alignment.
            if (q->getType() == objInt) { 
                if(q->getInt() == 1) {
                    text_align = "center";
                } else if(q->getInt() == 2) {
                    text_align = "right";
                }
            }
            
            if (lo->getType() == objString) {
                
                //cerr << "Flags: " << fflo->getType() << endl; 
                char *name = lo->getString()->getCString();
                cerr << "Writing text box for " << name << endl;
                out << "<div id=\"text-" << pageNum << "-" << i
                    << "\" form-field=\"" << name
                    << "\" class=\"" << classes 
                    << "\" style=\"position: absolute; font-family:arial; left: " << x1 
                    << "px; bottom: " << y1 << "px;" 
                    << " text-align: " << text_align << ";"
                    << " width: " << width << "px; height: " << std::to_string(height) 
                    << "px; line-height: " << std::to_string(height) << "px; font-size: " 
                    << font_size << font_unit << ";\" ></div>" << endl;
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
