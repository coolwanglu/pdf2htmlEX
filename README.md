# pdf2html**EX** 

[![Click here to lend your support to: pdf2htmlEX and make a donation at www.pledgie.com !]['http://www.pledgie.com/campaigns/18320.png?skin_name=chrome']](http://www.pledgie.com/campaigns/18320)

A beautiful demo is worth a thousand words:

[**Typography**](http://coolwanglu.github.com/pdf2htmlEX/demo/geneve.html) [Original](https://github.com/raphink/geneve_1564/raw/master/geneve_1564.pdf)

[**Formulas**](http://coolwanglu.github.com/pdf2htmlEX/demo/cheat.html) [Original](http://www.tug.org/texshowcase/cheat.pdf)

[**Scientific Paper**](http://coolwanglu.github.com/pdf2htmlEX/demo/demo.html) [Original](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.148.349&rep=rep1&type=pdf)

[**Chinese**](http://coolwanglu.github.com/pdf2htmlEX/demo/chn.html) [Original](http://files.cnblogs.com/phphuaibei/git%E6%90%AD%E5%BB%BA.pdf)


**WINDOWS XP USERS: Please make sure ClearType is turned on** 

(Control Panel -> Display -> Appearance -> Effects -> "Use the following method to smooth edges of screen fonts" -> ClearType)

## Introduction

pdf2htmlEX renders PDF files in HTML, utilizing modern Web technologies, aims to provide an accuracy rendering, while keeping optimized for Web display.

It is optimized for modern web browsers.On Linux/Mac, the generated HTML pages could be as beautiful as PDF files.

This program is designed for scientific papers with complicate formulas and figures, therefore precise rendering is the #1 concern. But of course general PDF files are also supported.

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

[PPA](https://launchpad.net/~coolwanglu/+archive/pdf2htmlex), which is not so up-to-date.

### ArchLinux

[AUR Package](https://aur.archlinux.org/packages.php?ID=62426), special thanks to Arthur Titeica <arthur.titeica@gmail.com>

### Mac

MacPorts and Homebrew are coming soon

### Windows

I have tested with CYGWIN without any problem, and I believe it also works on MinGW without many modifications.

### Build from source

#### Dependency

* CMake, pkg-config
* GNU Getopt
* compilers support C++11, for example
 * GCC >= 4.4.6
 * I heard about successful build with Clang 
* poppler with xpdf header >= 0.20.0 (compile with --enable-xpdf-headers)
 * Install libpng (and headers) BEFORE you compile poppler if you want background images generated
 * Install poppler-data if your want CJK support
* fontforge (with header files)
 * git version is recommended to avoid annoying compilation issues
* [Optional] ttfautohint
 * run pdf2htmlEX with --external-hint-tool=ttfautohint to enable it

#### Compiling

    cmake . && make && sudo make install

## Usage

    pdf2htmlEX /path/to/foobar.pdf

    pdf2htmlEX --help

## FAQ

[here](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ)

## LICENSE

GPLv2 & GPLv3 Dual licensed

**pdf2htmlEX is totally free, please credit pdf2htmlEX if you use it**

**Please consider sponsoring it if you use it for commercial purpose**

**Font extraction, conversion or redistribution may be illegal, please check your local laws**

## Credits

pdf2htmlEX is made possible thanks to the following projects:

* [poppler](http://poppler.freedesktop.org/)
* [Fontforge](http://fontforge.org/)
* [jQuery](http://jquery.com/)

pdf2htmlEX is inspired by the following projects:

* pdftops & pdftohtml from poppler 
* MuPDF
* PDF.js
* Crocodoc
* Google Doc

## Contact

Suggestions and questions are welcome. 

Please read [FAQ](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ) before sending an email to me. Or your message might be ignored.

I should be much more user-friendly than pdf2htmlEX.

* Lu Wang <coolwanglu@gmail.com>
  * Messages in Chinese, English or Japanese are accepted

### Special Thanks

* Hongliang Tian <tatetian@gmail.com>

