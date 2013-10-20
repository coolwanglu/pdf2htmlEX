#!/bin/sh
# Compile and optimize JS code
# Copyright 2013 Lu Wang <coolwanglu@gmail.com>

# To enable closure-compiler, you need to install and configure JAVA environment
# Read 3rdparty/closure-compiler/README for details


BASEDIR=$(dirname $0)
OUTPUT="$BASEDIR/../pdf2htmlEX.js"

(echo 'Compiling pdf2htmlEX.js with closure-compiler...' && \
    tmpfile=$(mktemp 2>/dev/null) && \
    java -jar "$BASEDIR/../../3rdparty/closure-compiler/compiler.jar" --compilation_level ADVANCED_OPTIMIZATIONS --process_jquery_primitives --externs "$BASEDIR/../jquery.js" --js "$BASEDIR/css_class_names.js" --js "$BASEDIR/viewer.js" > "$tmpfile" 2>/dev/null && \
    cat "$BASEDIR/header.js" "$tmpfile" > "$OUTPUT" && \
    rm -f -- "$tmpfile" && \
    echo 'Done.') || \
(echo 'Failed. Read `3rdparty/closure-compiler/README` for more detail.' && \
echo 'Fall back to naive concatenation' && \
cat "$BASEDIR/header.js" "$BASEDIR/css_class_names.js" "$BASEDIR/viewer.js" > "$OUTPUT")
