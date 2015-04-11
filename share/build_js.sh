#!/bin/sh -ex
# Compile and optimize JS code
# Copyright 2013 Lu Wang <coolwanglu@gmail.com>

# To enable closure-compiler, you need to install and configure JAVA environment
# Read 3rdparty/closure-compiler/README for details


BASEDIR=$(dirname $0)
CLOSURE_COMPILER_DIR="$BASEDIR/../3rdparty/closure-compiler"
CLOSURE_COMPILER_JAR="$CLOSURE_COMPILER_DIR/compiler.jar"
build () {
    INPUT="$BASEDIR/$1"
    OUTPUT_FN="$2"
    OUTPUT="$BASEDIR/$OUTPUT_FN"

    (echo "Building $OUTPUT_FN with closure-compiler..." && \
        java -jar "$CLOSURE_COMPILER_JAR" \
             --compilation_level $3 \
             --warning_level VERBOSE \
             --output_wrapper "(function(){%output%})();" \
             --js "$INPUT" \
             --js_output_file "$OUTPUT" && \
        echo 'Done.') || \
    (echo 'Failed. Read `3rdparty/closure-compiler/README` for more detail.' && \
    echo 'Using the uncompressed version.' && \
    cat "$INPUT" > "$OUTPUT")
}

build "pdf2htmlEX.js.in" "pdf2htmlEX.min.js" "ADVANCED_OPTIMIZATIONS"
build "navbar.js.in" "navbar.min.js" "SIMPLE_OPTIMIZATIONS"
