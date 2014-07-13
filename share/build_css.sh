#!/bin/sh -ex
# Compile and optimize CSS code
# Copyright 2013 Lu Wang <coolwanglu@gmail.com>


BASEDIR=$(dirname $0)
YUI_DIR="$BASEDIR/../3rdparty/yuicompressor"
YUI_JAR="$YUI_DIR/yuicompressor-2.4.8.jar"

build () {
    INPUT="$BASEDIR/$1"
    OUTPUT="$BASEDIR/$2"
    (echo "Building $OUTPUT with YUI Compressor" && \
        java -jar "$YUI_JAR" \
             --charset utf-8 \
             -o "$OUTPUT" \
             "$INPUT" && \
        echo 'Done.') || \
    (echo 'Failed. ' && \
    echo 'Using the uncompressed version.' && \
    cat "$INPUT" > "$OUTPUT")
}

build "base.css" "base.min.css"
build "fancy.css" "fancy.min.css"
