pdf2html**EX**
=============================


[**View Demo**](http://coolwanglu.github.com/pdf2htmlEX/demo/demo.html)

Introduction
-----------------------------
pdf2htmlEX renders PDF files in HTML, utilizing modern Web technologies, aims to provide an accuracy rendering, while keeping optimized for Web display.

It is optimized for recent versions of modern web browsers such as Mozilla Firefox & Google Chrome.

This program is designed for scientific papers with complicate formulas and figures, so a precise rendering is also the #1 concern.

Features
----------------------------
* Single HTML file output 
* Precise rendering 
* Text Selection
* Font embedding 
* Proper styling (Color, Transformation...)
* Optimization for Web 

Not supported yet
----------------------------
* Several Font types & encodings
* Non-text object (Don't worry, they will be rendered as images)
* Blend Mode
* ...

Dependency
----------------------------
* Recent version of GCC (no guarantee on other compilers)
* libpoppler with xpdf header >= 0.20.2
* boost c++ library (format, program options, gil, filesystem, serialization, system(which is actually required by filesystem))
* fontforge **Please use [the lastest version](https://github.com/fontforge/fontforge)**

HOW TO COMPILE
----------------------------
    cmake . && make && sudo make install

HOW TO USE
----------------------------
    pdf2htmlEX /path/to/sample.pdf

LICENSE
----------------------------
GPLv3


We would like to acknowledge the following projects that have been consulted while writing this program:
* pdftops & pdftohtml from poppler 
* MuPDF
* PDF.js
* Crocodoc
* Google Doc

AUTHORS
----------------------------
* Lu Wang <coolwanglu@gmail.com>
* Hongliang Tian <tatetian@gmail.com>

