#!/bin/sh -ex

# convert raw SVG into png of different sizes
convert -background none -resize 64x64^ pdf2htmlEX.svg pdf2htmlEX-64x64.png
convert -background none -resize 256x256^ pdf2htmlEX.svg pdf2htmlEX-256x256.png
# optimize for web
pngnq pdf2htmlEX-64x64.png
mv pdf2htmlEX-64x64-nq8.png ../share/pdf2htmlEX-64x64.png
