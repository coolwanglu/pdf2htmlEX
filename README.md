# pdf2html**EX** 

### [**Donate Now**](http://coolwanglu.github.com/pdf2htmlEX/donate.html)

### [**Feature Commission**](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ#wiki-feature_commission) are now accepted.

A beautiful demo is worth a thousand words:

[**Typography**](http://coolwanglu.github.com/pdf2htmlEX/demo/geneve.html) [Original](https://github.com/raphink/geneve_1564/raw/master/geneve_1564.pdf)

[**Full Circle Magazine(large)**](http://coolwanglu.github.com/pdf2htmlEX/demo/issue65_en.html) [Sample](http://coolwanglu.github.com/pdf2htmlEX/demo/issue65_en_sample.html) [Original](http://dl.fullcirclemagazine.org/issue65_en.pdf)

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
* [EXPERIMENTAL] Path drawing with CSS
 * Orthogonal lines
 * Rectangles
 * Linear gradients 

### Not supported yet

* Type 3 fonts
* Non-text object (Don't worry, they will be rendered as images)

## Get started

### Ubuntu 

[PPA](https://launchpad.net/~coolwanglu/+archive/pdf2htmlex), which is not so up-to-date.

### ArchLinux

[AUR Package](https://aur.archlinux.org/packages.php?ID=62426), special thanks to Arthur Titeica <arthur.titeica@gmail.com>

### Mac

[Homebrew Formula](https://github.com/jamiely/homebrew/blob/pdf2htmlex/Library/Formula/pdf2htmlex.rb), special thanks to Jamie Ly <me@jamie.ly>

[Macports (local repo)](https://github.com/iapain/pdf2htmlEX-macport), special thanks to Deepak Thukral <iapain@iapa.in>

### Windows

The code may be built with Cygwin.

Or with MinGW with [some modifications](http://oku.edu.mie-u.ac.jp/~okumura/texwiki/?pdf2htmlEX) (Japanese site)

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
    
    man pdf2htmlEX

### For Geeks

* Experimental and unsupported

    pdf2htmlEX --process-nontext 0 --css-draw 1 /path/to/foobar.pdf

## FAQ

* [Troubleshooting compilation errors](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ#wiki-compile)
* [How can I help](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ#wiki-help)
* [I want more features](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ#wiki-feature_commission)
* [More](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ)

## LICENSE

GPLv2 & GPLv3 Dual licensed

**pdf2htmlEX is totally free, please credit pdf2htmlEX if you use it**

**Please consider sponsoring it if you use it for commercial purpose**

**Font extraction, conversion or redistribution may be illegal, please check your local laws**

## Acknowledge

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

Special thanks to Haruhiko Okumura, for the [pdf2htmlEX page in TeX Wiki](http://oku.edu.mie-u.ac.jp/~okumura/texwiki/?pdf2htmlEX) (in Japanese).


## Contact

* Lu Wang <coolwanglu@gmail.com>
  * Suggestions and questions are **welcome**. 
  * Please read [**FAQ**](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ) before sending an email to me. Or your message might be ignored.
  * Please use the **latest master branch**.
  * Expect me to be much more user-friendly than pdf2htmlEX.
  * Accepting messages in **Chinese**, **English** or **Japanese**.


### Special Thanks

* Hongliang Tian <tatetian@gmail.com>

