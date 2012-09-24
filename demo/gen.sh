for f in ../test/demo/*.pdf; do pdf2htmlEX --external-hint-tool=ttfautohint --auto-hint=1 --zoom 2 $f; done
