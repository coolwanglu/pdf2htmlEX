#!/usr/bin/env python

DIR = 'pdf'
#DIR = '../../pdf.js/test/pdfs'

import os
import sys

with open('out.html','w') as outf:
    outf.write('<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"></head><body><div style="position:absolute;top:0;left:0;width:80%;height:100%;"><iframe width="100%" height="100%" name="pdf"></iframe></div><div style="position:absolute;top:0;right:0;width:20%;height:100%;overflow:auto;text-align:right;">')

    for f in os.listdir(DIR):
        if not f.lower().endswith('.pdf'):
            continue
        print f
        if os.system('pdf2htmlEX -l 10 --no-drm 1 --fit-width 1024 --dest-dir html --external-hint-tool="ttfautohint" "%s/%s"' % (DIR,f)) != 0:
            print "error on ", f
            sys.exit(-1)

        ff = f[:-3]
        outf.write('<a href="html/%shtml" target="pdf">%s</a><br/>' % (ff,ff))
        outf.flush();

    outf.write('</div></body></html>')
