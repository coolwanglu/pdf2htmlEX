/*
 * outline.cc
 *
 * Handling Outline items
 *
 * by WangLu
 * 2013.01.28
 */

#include <iostream>

#include <Outline.h>
#include <goo/GooList.h>

#include "HTMLRenderer.h"
#include "util/namespace.h"
#include "util/encoding.h"
#include "util/css_const.h"

namespace pdf2htmlEX {

using std::ostream;

void HTMLRenderer::process_outline_items(GooList * items)
{
    if((!items) || (items->getLength() == 0))
        return;

    f_outline.fs << "<ul>";

    for(int i = 0; i < items->getLength(); ++i)
    {
        OutlineItem * item = (OutlineItem*)(items->get(i));

        string detail;
        string dest = get_linkaction_str(item->getAction(), detail);

        // we don't care dest is empty or not.
        f_outline.fs << "<li>" << "<a class=\"" << CSS::LINK_CN << "\" href=\"";
        writeAttribute(f_outline.fs, dest);
        f_outline.fs << "\"";

        if(!detail.empty())
            f_outline.fs << " data-dest-detail='" << detail << "'";

        f_outline.fs << ">";

        writeUnicodes(f_outline.fs, item->getTitle(), item->getTitleLength());

        f_outline.fs << "</a>";

        // check kids
        item->open();
        if(item->hasKids())
        {
            process_outline_items(item->getKids());
        }
        item->close();
        f_outline.fs << "</li>";
    }

    f_outline.fs << "</ul>";
}
   
void HTMLRenderer::process_outline()
{
    Outline * outline = cur_doc->getOutline();
    if(!outline)
        return;

    process_outline_items(outline->getItems());
}

}// namespace pdf2htmlEX
