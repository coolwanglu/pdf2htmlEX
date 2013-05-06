#!/bin/bash

PARA="--optimize-text 0 --split-pages 1 --external-hint-tool=ttfautohint --auto-hint=1 --fit-width 1024"

for f in ../test/demo/*.pdf; do 
    pdf2htmlEX $PARA $f
done

#pdf2htmlEX $PARA -l 3 ../test/demo/issue65_en.pdf issue65_en_sample.html
