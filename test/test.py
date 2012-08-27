#!/usr/bin/env python

import os

outf = open('out.html','w')

outf.write('<html><body><div style="position:absolute;top:0;left:0;width:80%;height:100%;"><iframe width="100%" height="100%" name="pdf"></iframe></div><div style="position:absolute;top:0;right:0;width:21%;height:100%;">')

for f in os.listdir('pdf'):
    if not f.lower().endswith('.pdf'):
        continue
    print f
    os.system('pdf2htmlEX -l 3 --dest-dir html pdf/%s' % (f,))
    ff = f[:-3]+'html'
    outf.write('<a href="html/%s" target="pdf">%s</a><br/>' % (ff,ff))
    outf.flush();

outf.write('</div></body></html>')
