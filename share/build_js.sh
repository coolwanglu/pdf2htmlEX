#!/bin/sh
# Compile and optimize JS code
# Copyright 2013 Lu Wang <coolwanglu@gmail.com>

# To enable closure-compiler, you need to install and configure JAVA environment
# Read 3rdparty/closure-compiler/README for details


BASEDIR=$(dirname $0)
CLOSURE_COMPILER_DIR="$BASEDIR/../3rdparty/closure-compiler"
CLOSURE_COMPILER_JAR="$CLOSURE_COMPILER_DIR/compiler.jar"
EXTERNS="$CLOSURE_COMPILER_DIR/jquery-1.9.js"
INPUT="$BASEDIR/pdf2htmlEX.js"
OUTPUT_FN="pdf2htmlEX.min.js"
OUTPUT="$BASEDIR/$OUTPUT_FN"

(echo "Building $OUTPUT_FN with closure-compiler..." && \
    java -jar "$CLOSURE_COMPILER_JAR" \
         --compilation_level ADVANCED_OPTIMIZATIONS \
         --warning_level VERBOSE \
         --process_jquery_primitives \
         --externs "$EXTERNS" \
         --js "$INPUT" \
         --js_output_file "$OUTPUT" && \
    echo 'Done.') || \
(echo 'Failed. Read `3rdparty/closure-compiler/README` for more detail.' && \
echo 'Using the uncompressed version.' && \
cat "$INPUT" > "$OUTPUT")

