# pdf2htmlEX 

[![Build Status](https://travis-ci.org/coolwanglu/pdf2htmlEX.png?branch=master)](https://travis-ci.org/coolwanglu/pdf2htmlEX)

A beautiful demo is worth a thousand words:

- **Typography**: [Default](http://coolwanglu.github.com/pdf2htmlEX/demo/geneve.html) / [MediaFire](http://www.mediafire.com/view/?fqbc2d2o1kdz51a) / [Original](https://github.com/raphink/geneve_1564/raw/master/geneve_1564.pdf)
- **Formulas**: [Default](http://coolwanglu.github.com/pdf2htmlEX/demo/cheat.html) / [MediaFire](http://www.mediafire.com/view/?84vdgrepkxclbq2) / [Original](http://www.tug.org/texshowcase/cheat.pdf)
- **Scientific Paper**: [Default](http://coolwanglu.github.com/pdf2htmlEX/demo/demo.html) / [MediaFire](http://www.mediafire.com/view/?6po429kz9czcga2) / [Original](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.148.349&rep=rep1&type=pdf)
- **Full Circle Magazine**: [Default](http://coolwanglu.github.com/pdf2htmlEX/demo/issue65_en.html) / [MediaFire](http://www.mediafire.com/view/?6hxmt94k2vppnpb) / [Original](http://dl.fullcirclemagazine.org/issue65_en.pdf)   <sub>The 1st link might be slow</sub>
- **Chinese**: [Default](http://coolwanglu.github.com/pdf2htmlEX/demo/chn.html) / [MediaFire](http://www.mediafire.com/view/?6550ldag9w0uuq3) / [Original](http://files.cnblogs.com/phphuaibei/git%E6%90%AD%E5%BB%BA.pdf)
- Try your own files on [MediaFire](http://www.mediafire.com), which uses pdf2htmlEX for its PDF preview feature.
  
## Introduction

pdf2htmlEX renders PDF files in HTML, utilizing modern Web technologies.
It aims to provide an accuracy rendering, while keeping optimized for Web display.

pdf2htmlEX is best for text-based PDF files, for example scientific papers with complicated formulas and figures.
Text, fonts and formats are natively perserved in HTML such that you can still search and copy.
The generated HTML file is static, Javascript is not required.

[Learn more](https://github.com/coolwanglu/pdf2htmlEX/wiki/Introduction)

## Features

* Precise, native text in HTML, which means
  - You can select & copy & search
  - Correct font & position & styles
  - Proper reencoding
  - Generated HTML file is of similar size as the original (uncompressed) PDF file
* Output modes
  - Normal HTML
  - All-in-one HTML - portable & easy to share
  - One HTML per page - best for dynamic pages
* More PDF stuffs that you love
  - Links
  - Outline
  - Printing (experimental)

[Full list](https://github.com/coolwanglu/pdf2htmlEX/wiki/Feature-List)   
[Compare with others](https://github.com/coolwanglu/pdf2htmlEX/wiki/Comparison)

## Get started

### Install
 
Thanks to all packagers!

  * [Ubuntu PPA](https://launchpad.net/~coolwanglu/+archive/pdf2htmlex) by Lu Wang <coolwanglu@gmail.com>, not always up-to-date.
  * [ArchLinux AUR](https://aur.archlinux.org/packages.php?ID=62426) by Arthur Titeica <arthur.titeica@gmail.com>
  * [Gentoo Overlay](http://gpo.zugaina.org/app-text/pdf2htmlex), gentoo-zh, mrueg or sunrise, by respective packagers.  
  * [Homebrew Formula](https://github.com/mxcl/homebrew/blob/master/Library/Formula/pdf2htmlex.rb) by Jamie Ly <me@jamie.ly>
  * [Macports (local repo)](https://github.com/iapain/pdf2htmlEX-macport) by Deepak Thukral <iapain@iapa.in>
  * Windows [N/A](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ#wiki-install-windows)

### Build from source

#### Dependency

* CMake, pkg-config
* GNU Getopt
* compilers support C++11, for example
 * GCC >= 4.4.6
 * I heard about successful build with Clang 
* **poppler** with xpdf header >= 0.20.0 (compile with **--enable-xpdf-headers**)
 * Install **libpng** (and headers) BEFORE you compile poppler if you want background images generated
 * Install **poppler-data** if your want CJK support
* **fontforge** (with header files)
 * git version is recommended to avoid annoying compilation issues
* [Optional] **ttfautohint**
 * run pdf2htmlEX with **--external-hint-tool=ttfautohint** to enable it
* [For Windows]
 * Cygwin 
 * or MinGW, with some modifications to pdf2htmlEX. See [pdf2htmlEX on TeX Wiki](http://oku.edu.mie-u.ac.jp/~okumura/texwiki/?pdf2htmlEX) (in Japanese), special thanks to Haruhiko Okumura


#### Compiling

    git clone --depth 1 git://github.com/coolwanglu/pdf2htmlEX.git
    cd pdf2htmlEX
    cmake . && make && sudo make install

## Usage

    pdf2htmlEX /path/to/foobar.pdf
    pdf2htmlEX --help
    man pdf2htmlEX

## FAQ

* [Troubleshooting compilation errors](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ#wiki-compile)
* [How can I help](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ#wiki-help)
* [I want more features](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ#wiki-feature_commission)
* [More about pdf2htmlEX](https://github.com/coolwanglu/pdf2htmlEX/wiki/)

## LICENSE

GPLv2 & GPLv3 for most part, MIT License for share/\*   
Read LICENSE for detail.

**pdf2htmlEX is totally free, please credit pdf2htmlEX if you use it**  
**Please consider sponsoring it if you use it for commercial purpose**

**Font extraction, conversion or redistribution MAY BE ILLEGAL, please check your local laws**

## [**Donate Now**](http://coolwanglu.github.com/pdf2htmlEX/donate.html)

pdf2htmlEX is maintained by one person in spare time, and it needs your help!

## Contact

* Mailing list <pdf2htmlex@googlegroups.com>
  * You might want to try these useful resources first: `man pdf2htmlEX`, [wiki](https://github.com/coolwanglu/pdf2htmlEX/wiki) and [FAQ](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ)

* Lu Wang <coolwanglu@gmail.com>
  * For personal enquiries only
  * Accepting messages in **Chinese**, **English** or **Japanese**.

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

### Special Thanks

* Hongliang Tian
* Wanmin Liu 

