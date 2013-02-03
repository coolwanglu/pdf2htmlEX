#ifndef HTMLRENDERER_MP_H_
#define HTMLRENDERER_MP_H_

#include <iostream>
#include <sstream>
#include <deque>

#include <Page.h>
#include <Link.h>
#include <Outline.h>
#include <goo/GooList.h>

#include "HTMLRenderer/HTMLRenderer.h"
#include "util/unicode.h"
#include "util/base64stream.h"
#include "util/math.h"

namespace pdf2htmlEX {

// multipages version of HTMLRenderer
class HTMLRenderer_mp : public HTMLRenderer
{
public:
    HTMLRenderer_mp (const Param * param)
        : HTMLRenderer(param)
    { }

    // dump mediabox for each page
    void dump_mediabox(PDFDoc * doc) {
        for(int i = 1; i <= doc->getNumPages(); ++i) {
            auto * box = doc->getCatalog()->getPage(i)->getMediaBox();
            std::cerr << "Page " << i << ": " << box->x1 << ' ' << box->y1 << ' ' << box->x2 << ' ' << box->y2 << std::endl;
        }
    }

    // all pages indexed in outlines, sorted
    std::deque<int> outline_indexed_pages;

    // return the page number of the pages pointed by `action`
    // return -1 if there is no such page
    int get_pageno_from_linkaction(PDFDoc * doc, LinkAction * action) {
        int pageno = -1;
        if(action && (action->getKind() == actionGoTo)) {
            auto * real_action = dynamic_cast<LinkGoTo*>(action);
            LinkDest * dest = nullptr;
            if(auto _ = real_action->getDest()) dest = _->copy();
            else if (auto _ = real_action->getNamedDest()) dest = doc->getCatalog()->findDest(_);
            if(dest) {
                if(dest->isPageRef()) {
                    auto pageref = dest->getPageRef();
                    pageno = doc->getCatalog()->findPage(pageref.num, pageref.gen);
                } else {
                    pageno = dest->getPageNum();
                }
            }
        }
        return pageno;
    }

    // process all outline `items` and their descendants
    // also dump log to `out`
    void pre_process_outline_items(std::ostream & out, PDFDoc * doc, GooList * items) {
        if((!items) || (items->getLength() == 0)) return;
        for(int i = 0; i < items->getLength(); ++i) {
            auto * item = (OutlineItem*)(items->get(i));
            int pageno = get_pageno_from_linkaction(doc, item->getAction());
            if((pageno > 0)
                &&((pageno >= param->first_page) && (pageno <= param->last_page))
                &&(outline_indexed_pages.empty()||(pageno > outline_indexed_pages.back()))
              ) {
                outline_indexed_pages.push_back(pageno);
                out << pageno << ' ';
                outputUnicodes(out, item->getTitle(), item->getTitleLength());
                out << std::endl;
            }
            item->open();
            if(item->hasKids()) pre_process_outline_items(out, doc, item->getKids());
            item->close();
        }
    }

    // process the outline and dump the list of indexed pages 
    void pre_process_outline (PDFDoc * doc) {
        outline_indexed_pages.clear();
        std::ofstream fout((char*)str_fmt("%s/outline_index_page_list", param->dest_dir.c_str()), std::ofstream::binary);
        Outline * outline = doc->getOutline();
        if(!outline) return;
        pre_process_outline_items(fout, doc, outline->getItems());
    }

    virtual void process(PDFDoc * doc) {
        if(param->dump_mediabox) {
            dump_mediabox(doc);
        } else {
            if(!param->set_mediabox.empty()) {
                if(param->use_cropbox) {
                    std::cerr << "Warning: Both set_mediabox and use_cropbox are specified." << std::endl;
                }
                double x1,y1,x2,y2;
                std::istringstream sin(param->set_mediabox);
                if(!(sin >> x1 >> y1 >> x2 >> y2)) {
                    std::cerr << "Cannot parse mediabox: \"" << param->set_mediabox << "\"" << std::endl;
                    return;
                }
                // override the original media boxes
                PDFRectangle new_media_box(x1, y1, x2, y2);
                for(int i = 1; i <= doc->getNumPages(); ++i) {
                    auto * page = doc->getCatalog()->getPage(i);
                    doc->replacePageDict(i, 
                            page->getRotate(), 
                            &new_media_box,
                            (page->isCropped() ? page->getCropBox() : nullptr),
                            nullptr);
                    *(page->getMediaBox()) = new_media_box;
                }
            }
            if(param->group_by_outline) pre_process_outline(doc);
            HTMLRenderer::process(doc);
        }
    }

    virtual void post_process (void) {
        // dump info for js
        // BE CAREFUL WITH ESCAPES
        f_pages.fs << "<div class=\"j\" data-data='{";

        // groups
        if(param->group_by_outline){
            f_pages.fs << "\"groups\":[";
            for(auto iter = outline_indexed_pages.begin(); iter != outline_indexed_pages.end(); ++iter) {
                if(iter != outline_indexed_pages.begin()) f_pages.fs << ",";
                f_pages.fs << "\"" << (*iter) << "\"";
            }
            f_pages.fs << "]";
        }

        f_pages.fs << "}'></div>";

        HTMLRenderer::post_process();
    }
};

}

#endif //HTMLRENDERER_MP_H_
