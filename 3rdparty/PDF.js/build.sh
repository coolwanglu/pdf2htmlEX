#!/bin/sh
# Compile and optimize JS code
# Copyright 2013 Lu Wang <coolwanglu@gmail.com>


BASEDIR=$(dirname $0)
CLOSURE_COMPILER_DIR="$BASEDIR/../closure-compiler"
CLOSURE_COMPILER_JAR="$CLOSURE_COMPILER_DIR/compiler.jar"
INPUT="$BASEDIR/compatibility.js"
OUTPUT_FN="compatibility.min.js"
OUTPUT="$BASEDIR/$OUTPUT_FN"

(echo "Building $OUTPUT_FN with closure-compiler..." && \
    java -jar "$CLOSURE_COMPILER_JAR" \
         --compilation_level ADVANCED_OPTIMIZATIONS \
         --warning_level VERBOSE \
         --js "$INPUT" \
         --js_output_file "$OUTPUT" && \
    echo 'Done.') || \
(echo 'Failed. Read `3rdparty/closure-compiler/README` for more detail.' && \
echo 'Using the uncompressed version.' && \
cat "$INPUT" > "$OUTPUT")

