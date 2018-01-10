# leave this above project(pdf2htmlEX)
# set default build type to Release
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)")

project(pdf2htmlEX)
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

option(ENABLE_SVG "Enable SVG support, for generating SVG background images and converting Type 3 fonts" ON)

include_directories(${CMAKE_SOURCE_DIR}/src)

set(PDF2HTMLEX_VERSION "0.15.0")
set(ARCHIVE_NAME pdf2htmlex-${PDF2HTMLEX_VERSION})
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(PkgConfig)


pkg_check_modules(POPPLER REQUIRED poppler>=0.25.0)
include_directories(${POPPLER_INCLUDE_DIRS})
link_directories(${POPPLER_LIBRARY_DIRS})
set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${POPPLER_LIBRARIES})

if(ENABLE_SVG)
    pkg_check_modules(CAIRO REQUIRED cairo>=1.10.0)
    message("Trying to locate cairo-svg...")
    find_path(CAIRO_SVG_INCLUDE_PATH cairo-svg.h PATHS ${CAIRO_INCLUDE_DIRS} NO_DEFAULT_PATH)
    if(CAIRO_SVG_INCLUDE_PATH)
        include_directories(${CAIRO_INCLUDE_DIRS})
        link_directories(${CAIRO_LIBRARY_DIRS})
        set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${CAIRO_LIBRARIES})
        set(ENABLE_SVG 1)
        set(CAIROOUTPUTDEV_PATH 3rdparty/poppler/git)
        include_directories(${CAIROOUTPUTDEV_PATH})
        set(PDF2HTMLEX_SRC ${PDF2HTMLEX_SRC}
            ${CAIROOUTPUTDEV_PATH}/CairoFontEngine.h
            ${CAIROOUTPUTDEV_PATH}/CairoFontEngine.cc
            ${CAIROOUTPUTDEV_PATH}/CairoRescaleBox.h
            ${CAIROOUTPUTDEV_PATH}/CairoRescaleBox.cc
            ${CAIROOUTPUTDEV_PATH}/CairoOutputDev.h
            ${CAIROOUTPUTDEV_PATH}/CairoOutputDev.cc
        )
    else()
        message(FATAL_ERROR "Error: no SVG support found in Cairo")
    endif()

    find_package(Freetype REQUIRED)
    include_directories(${FREETYPE_INCLUDE_DIRS})
    link_directories(${FREETYPE_LIBRARY_DIRS})
    set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${FREETYPE_LIBRARIES})
endif()

# fontforge starts using pkg-config 'correctly' since 2.0.0
pkg_check_modules(FONTFORGE REQUIRED libfontforge>=2.0.0)
include_directories(${FONTFORGE_INCLUDE_DIRS})
link_directories(${FONTFORGE_LIBRARY_DIRS})
set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${FONTFORGE_LIBRARIES})

# debug build flags (overwrite default cmake debug flags)
set(CMAKE_C_FLAGS_DEBUG "-ggdb -pg")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -pg")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-pg")

# release build flags (overwrite default cmake release flags)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# generic flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# clang compiler need c++11 flag
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

# CYGWIN or GCC 4.5.x bug
if(CYGWIN)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -pthread")
endif()

# check the C++11 features we need
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <vector>
int main()
{
  char * ptr = nullptr;
  std::vector<int> v;
  auto f = [&](){ for(auto & i : v) ++i; };
  f();
}
" CXX0X_SUPPORT)
if(NOT CXX0X_SUPPORT)
    message(FATAL_ERROR "Error: your compiler does not support C++0x/C++11, please update it.")
endif()


configure_file (${CMAKE_SOURCE_DIR}/src/pdf2htmlEX-config.h.in ${CMAKE_SOURCE_DIR}/src/pdf2htmlEX-config.h)
configure_file (${CMAKE_SOURCE_DIR}/pdf2htmlEX.1.in ${CMAKE_SOURCE_DIR}/pdf2htmlEX.1)

include(${CMAKE_SOURCE_DIR}/src/css_class_names.cmakelists.txt)
configure_file (${CMAKE_SOURCE_DIR}/src/util/css_const.h.in ${CMAKE_SOURCE_DIR}/src/util/css_const.h)
configure_file (${CMAKE_SOURCE_DIR}/share/base.css.in ${CMAKE_SOURCE_DIR}/share/base.css)
configure_file (${CMAKE_SOURCE_DIR}/share/fancy.css.in ${CMAKE_SOURCE_DIR}/share/fancy.css)
configure_file (${CMAKE_SOURCE_DIR}/share/pdf2htmlEX.js.in ${CMAKE_SOURCE_DIR}/share/pdf2htmlEX.js)

set(PDF2HTMLEX_SRC ${PDF2HTMLEX_SRC}
    src/Param.h
    src/pdf2htmlEX.cc
    src/pdf2htmlEX-config.h
    src/HTMLRenderer/HTMLRenderer.h
    src/HTMLRenderer/draw.cc
    src/HTMLRenderer/general.cc
    src/HTMLRenderer/image.cc
    src/HTMLRenderer/font.cc
    src/HTMLRenderer/form.cc
    src/HTMLRenderer/link.cc
    src/HTMLRenderer/outline.cc
    src/HTMLRenderer/state.cc
    src/HTMLRenderer/text.cc
    src/BackgroundRenderer/BackgroundRenderer.h
    src/BackgroundRenderer/BackgroundRenderer.cc
    src/BackgroundRenderer/SplashBackgroundRenderer.h
    src/BackgroundRenderer/SplashBackgroundRenderer.cc
    src/BackgroundRenderer/CairoBackgroundRenderer.h
    src/BackgroundRenderer/CairoBackgroundRenderer.cc
    src/util/const.h
    src/util/const.cc
    src/util/css_const.h
    src/util/encoding.h
    src/util/encoding.cc
    src/util/ffw.h
    src/util/ffw.c
    src/util/math.h
    src/util/math.cc
    src/util/misc.h
    src/util/misc.cc
    src/util/namespace.h
    src/util/path.h
    src/util/path.cc
    src/util/unicode.h
    src/util/unicode.cc
    src/util/mingw.h
    src/util/mingw.cc
    src/ArgParser.h
    src/ArgParser.cc
    src/Base64Stream.h
    src/Base64Stream.cc
    src/Color.h
    src/Color.cc
    src/CoveredTextDetector.h
    src/CoveredTextDetector.cc
    src/DrawingTracer.h
    src/DrawingTracer.cc
    src/HTMLState.h
    src/HTMLTextLine.h
    src/HTMLTextLine.cc
    src/HTMLTextPage.h
    src/HTMLTextPage.cc
    src/Preprocessor.h
    src/Preprocessor.cc
    src/StringFormatter.h
    src/StringFormatter.cc
    src/TmpFiles.h
    src/TmpFiles.cc
    )

add_executable(pdf2htmlEX ${PDF2HTMLEX_SRC})
target_link_libraries(pdf2htmlEX ${PDF2HTMLEX_LIBS})

add_custom_target(pdf2htmlEX_resources ALL DEPENDS
    ${CMAKE_SOURCE_DIR}/share/base.min.css
    ${CMAKE_SOURCE_DIR}/share/fancy.min.css
    ${CMAKE_SOURCE_DIR}/share/pdf2htmlEX.min.js
    )

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/share/pdf2htmlEX.min.js
    COMMAND ${CMAKE_SOURCE_DIR}/share/build_js.sh
    DEPENDS
        ${CMAKE_SOURCE_DIR}/share/build_js.sh
        ${CMAKE_SOURCE_DIR}/share/pdf2htmlEX.js
    )

add_custom_command(OUTPUT
        ${CMAKE_SOURCE_DIR}/share/base.min.css
        ${CMAKE_SOURCE_DIR}/share/fancy.min.css
    COMMAND ${CMAKE_SOURCE_DIR}/share/build_css.sh
    DEPENDS
        ${CMAKE_SOURCE_DIR}/share/build_css.sh
        ${CMAKE_SOURCE_DIR}/share/base.css
        ${CMAKE_SOURCE_DIR}/share/fancy.css
    )

install (TARGETS pdf2htmlEX DESTINATION bin)

set(PDF2HTMLEX_RESOURCE
    ${CMAKE_SOURCE_DIR}/3rdparty/PDF.js/compatibility.js
    ${CMAKE_SOURCE_DIR}/3rdparty/PDF.js/compatibility.min.js
    ${CMAKE_SOURCE_DIR}/share/base.css
    ${CMAKE_SOURCE_DIR}/share/base.min.css
    ${CMAKE_SOURCE_DIR}/share/fancy.css
    ${CMAKE_SOURCE_DIR}/share/fancy.min.css
    ${CMAKE_SOURCE_DIR}/share/LICENSE
    ${CMAKE_SOURCE_DIR}/share/manifest
    ${CMAKE_SOURCE_DIR}/share/pdf2htmlEX.js
    ${CMAKE_SOURCE_DIR}/share/pdf2htmlEX.min.js
    ${CMAKE_SOURCE_DIR}/share/pdf2htmlEX-64x64.png
    )
install (FILES ${PDF2HTMLEX_RESOURCE} DESTINATION share/pdf2htmlEX)
install (FILES pdf2htmlEX.1 DESTINATION share/man/man1)

## tests:

set(PDF2HTMLEX_PATH ${CMAKE_BINARY_DIR}/pdf2htmlEX)
set(PDF2HTMLEX_TMPDIR /tmp/pdf2htmlEX/tmp)
set(PDF2HTMLEX_DATDIR /tmp/pdf2htmlEX/dat)
set(PDF2HTMLEX_PNGDIR /tmp/pdf2htmlEX/png)
set(PDF2HTMLEX_OUTDIR /tmp/pdf2htmlEX/out)
file(MAKE_DIRECTORY ${PDF2HTMLEX_TMPDIR})
file(MAKE_DIRECTORY ${PDF2HTMLEX_DATDIR})
file(MAKE_DIRECTORY ${PDF2HTMLEX_PNGDIR})
file(MAKE_DIRECTORY ${PDF2HTMLEX_OUTDIR})
configure_file(${CMAKE_SOURCE_DIR}/test/test.py.in ${CMAKE_SOURCE_DIR}/test/test.py)

include(CTest)
add_test(test_basic   python ${CMAKE_SOURCE_DIR}/test/test_output.py)
add_test(test_browser python ${CMAKE_SOURCE_DIR}/test/test_local_browser.py)
