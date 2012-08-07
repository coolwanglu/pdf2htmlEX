/*
 * Constants
 *
 * by WangLu
 * 2012.08.07
 */

#ifndef CONSTS_H__
#define CONSTS_H__
#include <string>
#include <map>

const char * HTML_HEAD = "<!DOCTYPE html>\n\
<html><head>\
<meta charset=\"utf-8\">\
<style type=\"text/css\">\
#pdf-main {\
  font-family: sans-serif;\
  position:absolute;\
  top:0;\
  left:0;\
  bottom:0;\
  right:0;\
  overflow:auto;\
  background-color:grey;\
}\
#pdf-main > .p {\
  position:relative;\
  margin:13px auto;\
  background-color:white;\
  overflow:hidden;\
  display:none;\
}\
.p  > .l {\
  position:absolute; \
  white-space:pre;\
}\
.l > .w {\
  display:inline-block;\
}\
::selection{\
  background: rgba(168,209,255,0.5);\
}\
::-moz-selection{\
  background: rgba(168,209,255,0.5);\
}\
</style><link rel=\"stylesheet\" type=\"text/css\" href=\"all.css\" />\
<script type=\"text/javascript\">\
function show_pages()\
{\
 var pages = document.getElementById('pdf-main').childNodes;\
 var idx = 0;\
 var f = function(){\
  if (idx < pages.length) {\
   try{\
    pages[idx].style.display='block';\
   }catch(e){}\
   ++idx;\
   setTimeout(f,100);\
  }\
 };\
 f();\
};\
</script>\
</head><body onload=\"show_pages();\"><div id=\"pdf-main\">";

const char * HTML_TAIL = "</div></body></html>";

const std::map<string, string> BASE_14_FONT_CSS_FONT_MAP({\
   { "Courier", "Courier,monospace" },\
   { "Helvetica", "Helvetica,Arial,\"Nimbus Sans L\",sans-serif" },\
   { "Times", "Times,\"Time New Roman\",\"Nimbus Roman No9 L\",serif" },\
   { "Symbol", "Symbol,\"Standard Symbols L\"" },\
   { "ZapfDingbats", "ZapfDingbats,\"Dingbats\"" },\
});

const double id_matrix[6] = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};

#endif //CONSTS_H__
