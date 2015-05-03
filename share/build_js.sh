#!/bin/sh -ex
# Compile and optimize JS code
# Copyright 2013 Lu Wang <coolwanglu@gmail.com>

# To enable closure-compiler, you need to install and configure JAVA environment
# Read 3rdparty/closure-compiler/README for details


BASEDIR=$(dirname $0)
CLOSURE_COMPILER_DIR="$BASEDIR/../3rdparty/closure-compiler"
CLOSURE_COMPILER_JAR="$CLOSURE_COMPILER_DIR/compiler.jar"
INPUT="$BASEDIR/pdf2htmlEX.js"
OUTPUT_FN="pdf2htmlEX.min.js"
OUTPUT="$BASEDIR/$OUTPUT_FN"

(echo "Building $OUTPUT_FN with closure-compiler..." && \
    java -jar "$CLOSURE_COMPILER_JAR" \
         --compilation_level SIMPLE_OPTIMIZATIONS \
         --warning_level VERBOSE \
         --output_wrapper "(function(){%output%})();" \
         --js "$INPUT" \
         --js_output_file "$OUTPUT" && \
    echo 'Done.') || \
(echo 'Failed. Read `3rdparty/closure-compiler/README` for more detail.' && \
echo 'Using the uncompressed version.' && \
cat "$INPUT" > "$OUTPUT")

