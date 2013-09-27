#!/bin/sh

convert -background none -resize 64x64^ pdf2htmlEX.svg pdf2htmlEX-64x64.png
convert -background none -resize 256x256^ pdf2htmlEX.svg pdf2htmlEX-256x256.png
convert -background none -resize 62x62^ pdf2htmlEX-plain.svg pdf2htmlEX-plain-62x62.png
convert -background none -resize 64x64^ pdf2htmlEX-shadow.svg pdf2htmlEX-shadow-64x64.png
