pdf2html**EX**
=============================


[**View Demo**](http://coolwanglu.github.com/pdf2htmlEX/demo/demo.html)

Introduction
-----------------------------
pdf2htmlEX renders PDF files in HTML, utilizing modern technologies of html/css, aims to provide an accuracy rendering, while keeping optimized for Web display.

pdf2htmlEX is optimized for recent versions of modern web browsers such as Mozilla Firefox & Google Chrome.

Features
----------------------------
* Text Selection
* Font embedding
* Proper styling (Color, Transformation...)
* Optimization for Web 

Not supported yet
----------------------------
* Non-text object (Don't worry, they will be rendered as images)
* Blend Mode
* CJK
* ...

Dependency
----------------------------
* libpoppler with xpdf header >= 0.20.2
* boost c++ library (format, program options)
* fontforge (we will compile with libfontforge later)

HOW TO COMPILE
----------------------------
    cmake . && make

HOW TO USE
----------------------------
    bin/pdf2htmlEX /path/to/sample.pdf


LICENSE
----------------------------
GPLv3


We would like to acknowledge the following projects that have been consulted while writing this program:
* pdftops & pdftohtml from poppler 
* PDF.js
* Crocodoc
* Google Doc

AUTHORS
----------------------------
* Lu Wang <coolwanglu@gmail.com>
* Hongliang Tian <tatetian@gmail.com>

