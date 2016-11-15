#![](http://coolwanglu.github.io/pdf2htmlEX/images/pdf2htmlEX-64x64.png) pdf2htmlEX 

# My branch differences:

This is my branch of pdf2htmlEX which I maintain for my own purposes. I have made a number of changes and improvements over the original code:

* Lots of bugs fixes, mostly of edge cases
* Integration of latest Cairo code
* Out of source building
* Rewritten handling of obscured/partially obscured text - now much more accurate
* Some support for transparent text
* Improvement of DPI settings - clamping of DPI to ensure output graphic isn't too big

`--correct-text-visibility` tracks the visibility of 4 sample points for each character (currently the 4 corners of the character's bounding box, inset slightly) to determine visibility.
It now has two modes. 1 = Fully occluded text handled (i.e. doesn't get put into the HTML layer). 2 = Partially occluded text handled.

The default is now "1", so fully occluded text should no longer show through. If "2" is selected then if the character is partially occluded it will be drawn in the background layer. In this case, the rendered DPI of the page will be automatically increased to `--covered-text-dpi` (default: 300) to reduce the impact of rasterized text.

For maximum accuracy I strongly recommend using the output options: `--font-size-multiplier 1 --zoom 25`. This will circumvent rounding errors inside web browsers. You will then have to sscale down the resulting HTML page using an appropriate "scale" transform.

If you are concerned about file size of the resulting HTML, then I recommend patching fontforge to prevent it writing the current time into the dumped fonts, and then post-process the pdf2htmlEX data to remove duplicate files - there will usually be many duplicate background images and fonts.

<!--
[![Build Status](https://travis-ci.org/coolwanglu/pdf2htmlEX.png?branch=master)](https://travis-ci.org/coolwanglu/pdf2htmlEX)
-->
>一图胜千言<br>A beautiful demo is worth a thousand words

- **Bible de Genève, 1564** (fonts and typography): [HTML](http://coolwanglu.github.com/pdf2htmlEX/demo/geneve.html) / [PDF](https://github.com/raphink/geneve_1564/releases/download/2015-07-08_01/geneve_1564.pdf)
- **Cheat Sheet** (math formulas): [HTML](http://coolwanglu.github.com/pdf2htmlEX/demo/cheat.html) / [PDF](http://www.tug.org/texshowcase/cheat.pdf)
- **Scientific Paper** (text and figures): [HTML](http://coolwanglu.github.com/pdf2htmlEX/demo/demo.html) / [PDF](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.148.349&rep=rep1&type=pdf)
- **Full Circle Magazine** (read while downloading): [HTML](http://coolwanglu.github.com/pdf2htmlEX/demo/issue65_en.html) / [PDF](http://dl.fullcirclemagazine.org/issue65_en.pdf)
- **Git Manual** (CJK support): [HTML](http://coolwanglu.github.com/pdf2htmlEX/demo/chn.html) / [PDF](http://files.cnblogs.com/phphuaibei/git%E6%90%AD%E5%BB%BA.pdf)

pdf2htmlEX renders PDF files in HTML, utilizing modern Web technologies.
Academic papers with lots of formulas and figures? Magazines with complicated layouts? No problem!

pdf2htmlEX is also an [online publishing tool](http://coolwanglu.github.io/pdf2htmlEX/doc/tb108wang.html) which is flexible for many different use cases. 

Learn more about [who](https://github.com/coolwanglu/pdf2htmlEX/wiki/Use-Cases) and [why](https://github.com/coolwanglu/pdf2htmlEX/wiki/Introduction) should use pdf2htmlEX.

### Features

* Native HTML text with precise font and location.
* Flexible output: all-in-one HTML or on demand page loading (needs JavaScript).
* Moderate file size, sometimes even smaller than PDF.
* Supporting links, outlines (bookmarks), printing, SVG background, Type 3 fonts and [more...](https://github.com/coolwanglu/pdf2htmlEX/wiki/Feature-List)

[Compare to others](https://github.com/coolwanglu/pdf2htmlEX/wiki/Comparison)

### Wiki Portals

 * [:house:Wiki Home](https://github.com/coolwanglu/pdf2htmlEX/wiki)
 * [Download](https://github.com/coolwanglu/pdf2htmlEX/wiki/Download) & [Building](https://github.com/coolwanglu/pdf2htmlEX/wiki/Building)
 * [Quick Start](https://github.com/coolwanglu/pdf2htmlEX/wiki/Quick-Start)
 * [Report Issues / Ask for Help](https://github.com/coolwanglu/pdf2htmlEX/blob/master/CONTRIBUTING.md#guidance)

### Get in Touch

Get quick answers for common questions:
 * [:question:FAQ](https://github.com/coolwanglu/pdf2htmlEX/wiki/FAQ)

Don't miss the latest development news:
 * [Development Blog](http://pdf2htmlex.blogspot.com) 
 * [@pdf2htmlEX](https://twitter.com/pdf2htmlEX) 

Discuss with the developers and the users of pdf2htmlEX
 * [:envelope:Mailing List](https://groups.google.com/forum/#!forum/pdf2htmlex)
 * [:mahjong:中文邮件列表](https://groups.google.com/forum/#!forum/pdf2htmlex-cn)
 
Chat with the main author: 王璐 (Lu Wang)
 * <coolwanglu@gmail.com>
 * [@coolwanglu](https://twitter.com/coolwanglu)
 * :bangbang:Questions about pdf2htmlEX? Use the mailling list instead.:bangbang:
 * Accepting messages in :cn::us::gb::jp: (languages).

Want to help without coding? Thank you!
* [:moneybag:Make a donation](http://coolwanglu.github.io/pdf2htmlEX/donate.html)
 
### LICENSE

pdf2htmlEX, as a whole package, is licensed under GPLv3+.
Some resource files are released with relaxed licenses, read `LICENSE` for more details.

### Acknowledgements

pdf2htmlEX is made possible thanks to the following projects:

* [poppler](http://poppler.freedesktop.org/)
* [Fontforge](http://fontforge.org/)

pdf2htmlEX is inspired by the following projects:

* pdftohtml from poppler 
* MuPDF
* PDF.js
* Crocodoc
* Google Doc

#### Special Thanks

* Hongliang Tian
* Wanmin Liu 

