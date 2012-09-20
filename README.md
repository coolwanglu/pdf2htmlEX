# pdf2html**EX**

A beautiful demo is worth a thousand words:

[**Typography**](http://coolwanglu.github.com/pdf2htmlEX/demo/geneve.html) [Original](https://github.com/raphink/geneve_1564/raw/master/geneve_1564.pdf)

[**Formulas**](http://coolwanglu.github.com/pdf2htmlEX/demo/cheat.html) [Original](http://www.tug.org/texshowcase/cheat.pdf)

[**Scientific Paper**](http://coolwanglu.github.com/pdf2htmlEX/demo/demo.html) [Original](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.148.349&rep=rep1&type=pdf)

[**Chinese**](http://coolwanglu.github.com/pdf2htmlEX/demo/chn.html) [Original](http://files.cnblogs.com/phphuaibei/git%E6%90%AD%E5%BB%BA.pdf)


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
* Links
* Optimization for Web 

### Not supported yet

* Type 3 fonts
* Non-text object (Don't worry, they will be rendered as images)

## Get started

### Ubuntu 

There is a Ubuntu PPA set up at [here](https://launchpad.net/~coolwanglu/+archive/pdf2htmlex).

### ArchLinux

Special thanks to Arthur Titeica for the [AUR Package](https://aur.archlinux.org/packages.php?ID=62426).

### Build from source

#### Dependency

* CMake, pkg-config
* compilers support C++11, for example
 * GCC >= 4.4.6
 * I heard about successful build with Clang 
* libpoppler with xpdf header >= 0.20.0 (compile with --enable-xpdf-headers)
 * Install libpng (and headers) BEFORE you compile libpoppler if you want background images generated
 * Install poppler-data if your want CJK support
* fontforge (with header files)

**Build On Windows**

I've tested with CYGWIN without any problem, and I believe it also works on MinGW without many modifications.

#### Compiling

    cmake . && make && sudo make install

## Usage

    pdf2htmlEX /path/to/foobar.pdf

    pdf2htmlEX --help

## LICENSE

GPLv2 & GPLv3 Dual licensed

**pdf2htmlEX is totally free, please credit pdf2htmlEX if you use it**

**Please consider sponsoring it if you use it for commercial purpose**

**Font extraction, conversion or redistribution may be illegal, please check your local laws**

## Credits

pdf2htmlEX is inspired by the following projects:

* pdftops & pdftohtml from poppler 
* MuPDF
* PDF.js
* Crocodoc
* Google Doc

## Contact

Suggestions and questions are welcome. 

Please read [FAQ](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ) before sending an email to me. Or your message might be ignored.

* Lu Wang <coolwanglu@gmail.com>

### Special Thanks

* Hongliang Tian <tatetian@gmail.com>

