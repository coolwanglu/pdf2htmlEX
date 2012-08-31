# pdf2html**EX**


[**View Demo**](http://coolwanglu.github.com/pdf2htmlEX/demo/demo.html)

[**Another Demo (CJK)**](http://coolwanglu.github.com/pdf2htmlEX/demo/chn.html)

**WINDOWS XP USERS: Please make sure ClearType is turned on** 

(Control Panel -> Display -> Appearance -> Effects -> "Use the following method to smooth edges of screen fonts" -> ClearType)

## Introduction

pdf2htmlEX renders PDF files in HTML, utilizing modern Web technologies, aims to provide an accuracy rendering, while keeping optimized for Web display.

It is optimized for modern web browsers such as Mozilla Firefox & Google Chrome.

This program is designed for scientific papers with complicate formulas and figures, so a precise rendering is also the #1 concern. But of course general PDF files are also supported.

## Features

* Single HTML file output 
* Precise rendering 
* Text Selection
* Font embedding & reencoding for Web
* Proper styling (Color, Transformation...)
* Optimization for Web 

### Not supported yet

* Type 3 fonts
* Non-text object (Don't worry, they will be rendered as images)

## Get started

### Ubuntu PPA

There is a Ubuntu PPA set up at [here](https://launchpad.net/~coolwanglu/+archive/pdf2htmlex).

Make sure you install fontforge in the PPA or [the git version](https://github.com/fontforge/fontforge).


### Build from source

#### Dependency

* CMake 
* compilers support C++11
* libpoppler with xpdf header >= 0.20.2
* boost c++ library (format, program options, gil, filesystem, serialization, system(which is actually required by filesystem))
* fontforge **Please use [the lastest version](https://github.com/fontforge/fontforge)**

#### Compiling

    cmake . && make && sudo make install

## Usage

    pdf2htmlEX /path/to/foobar.pdf

    pdf2htmlEX --help

## LICENSE

GPLv2 & GPLv3 Dual licensed

## Credits

The following projects have been consulted for pdf2htmlEX:

* pdftops & pdftohtml from poppler 
* MuPDF
* PDF.js
* Crocodoc
* Google Doc

## Contact

* Lu Wang <coolwanglu@gmail.com>

### Special Thanks

* Hongliang Tian <tatetian@gmail.com>

