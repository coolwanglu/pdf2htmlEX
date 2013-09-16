# leave this above project(pdf2htmlEX)
# set default build type to Release
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)")

project(pdf2htmlEX)
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

include_directories(${CMAKE_SOURCE_DIR}/src)

set(PDF2HTMLEX_VERSION "0.9")
set(ARCHIVE_NAME pdf2htmlex-${PDF2HTMLEX_VERSION})
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(PkgConfig)
pkg_check_modules(POPPLER REQUIRED poppler>=0.20.0)
include_directories(${POPPLER_INCLUDE_DIRS})
link_directories(${POPPLER_LIBRARY_DIRS})
set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${POPPLER_LIBRARIES})

# disable CAIRO for now
if(0)
pkg_check_modules(POPPLER_CAIRO poppler-cairo>=0.20.0)
if(POPPLER_CAIRO_FOUND)
    set(HAVE_CAIRO 1)
    include_directories(${POPPLER_CAIRO_INCLUDE_DIRS})
    link_directories(${POPPLER_CAIRO_LIBRARY_DIRS})
    set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${POPPLER_CAIRO_LIBRARIES})
else()
    set(HAVE_CAIRO 0)
endif()
else()
set(HAVE_CAIRO 0)
endif()
    

# fontforge starts using pkg-config 'correctly' since 2.0.0
pkg_check_modules(FONTFORGE libfontforge>=2.0.0)
if(FONTFORGE_FOUND)
    include_directories(${FONTFORGE_INCLUDE_DIRS})
    link_directories(${FONTFORGE_LIBRARY_DIRS})
    set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${FONTFORGE_LIBRARIES})
else()
    message("Trying to locate old versions of fontforge...")
    find_path(FF_INCLUDE_PATH fontforge/fontforge.h)
    if(FF_INCLUDE_PATH)
        message("Found fontforge.h: ${FF_INCLUDE_PATH}/fontforge/fontforge.h")
        set(FONTFORGE_INCLUDE_DIRS ${FF_INCLUDE_PATH}/fontforge)
        include_directories(${FONTFORGE_INCLUDE_DIRS})
        # MacOSX gettext is in /opt/local/include - strange
        if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")  
          find_path(GETTEXT_INCLUDE_PATH libintl.h HINTS "/usr/local/opt/gettext/include") # homebrew
          if(GETTEXT_INCLUDE_PATH)
            include_directories(${GETTEXT_INCLUDE_PATH})
          else()
            message("Cannot found libintl.h, if you see errors about libintl.h, add the path to gettext header files into your include paths")
          endif()
        endif()
    else()
        message(FATAL_ERROR "Error: cannot locate fontforge.h")
    endif()
    find_path(FF_CONFIG_INCLUDE_PATH config.h PATHS
      ${FONTFORGE_INCLUDE_DIRS} NO_DEFAULT_PATH)
    if(FF_CONFIG_INCLUDE_PATH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${FF_CONFIG_INCLUDE_PATH}/config.h")
    message("Found config.h: ${FF_CONFIG_INCLUDE_PATH}/config.h")
    else()
    message("Cannot locate config.h for fontforge")
    endif()

    macro(wl_find_library LIB_NAME RESULT)
        unset(${RESULT})
        unset(${RESULT} CACHE)
        foreach(FULL_LIB_NAME 
            ${CMAKE_IMPORT_LIBRARY_PREFIX}${LIB_NAME}${CMAKE_IMPORT_LIBRARY_SUFFIX}
            ${CMAKE_SHARED_LIBRARY_PREFIX}${LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${CMAKE_STATIC_LIBRARY_PREFIX}${LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
        )
            find_library(FULL_LIB ${FULL_LIB_NAME})
            if(FULL_LIB)
                message("Found ${LIB_NAME}: ${FULL_LIB}")
                set(${RESULT} ${FULL_LIB})
                break()
            endif()
        endforeach()
        unset(FULL_LIB_NAME)
        unset(FULL_LIB_NAME CACHE)
        unset(FULL_LIB)
        unset(FULL_LIB CACHE)
    endmacro()

    wl_find_library(fontforge FONTFORGE_LIBRARIES)
    if(NOT DEFINED FONTFORGE_LIBRARIES)
        set(FONTFORGE_LIBRARIES fontforge)
        message("Error: cannot locate fontforge")
    endif()
    set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${FONTFORGE_LIBRARIES})

    wl_find_library(gunicode GUNICODE_LIBRARIES)
    set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${GUNICODE_LIBRARIES})

    message("Looking for libraries of python, which is required by fontforge, if you can link fontforge without python, you may disable this")
    pkg_search_module(PYTHON python python-2.7)
    set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${PYTHON_LIBRARIES})
endif()


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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("${CMAKE_CXX_FLAGS}" CXX0X_SUPPORT)
if(NOT CXX0X_SUPPORT)
    message(FATAL_ERROR "Error: you compiler does not support C++0x, please update it.")
endif()


configure_file (${CMAKE_SOURCE_DIR}/src/pdf2htmlEX-config.h.in ${CMAKE_SOURCE_DIR}/src/pdf2htmlEX-config.h)
configure_file (${CMAKE_SOURCE_DIR}/pdf2htmlEX.1.in ${CMAKE_SOURCE_DIR}/pdf2htmlEX.1)

include(${CMAKE_SOURCE_DIR}/src/css_class_names.cmakelists.txt)
configure_file (${CMAKE_SOURCE_DIR}/src/util/css_const.h.in ${CMAKE_SOURCE_DIR}/src/util/css_const.h)
configure_file (${CMAKE_SOURCE_DIR}/share/base.css.in ${CMAKE_SOURCE_DIR}/share/base.css)
configure_file (${CMAKE_SOURCE_DIR}/share/fancy.css.in ${CMAKE_SOURCE_DIR}/share/fancy.css)
configure_file (${CMAKE_SOURCE_DIR}/share/pdf2htmlEX.js.in ${CMAKE_SOURCE_DIR}/share/pdf2htmlEX.js)

add_executable(pdf2htmlEX 
    src/Param.h
    src/pdf2htmlEX-config.h
    src/pdf2htmlEX.cc 
    src/HTMLRenderer/HTMLRenderer.h 
    src/HTMLRenderer/draw.cc
    src/HTMLRenderer/general.cc
    src/HTMLRenderer/image.cc
    src/HTMLRenderer/font.cc
    src/HTMLRenderer/link.cc
    src/HTMLRenderer/outline.cc
    src/HTMLRenderer/state.cc
    src/HTMLRenderer/text.cc
    src/BackgroundRenderer/BackgroundRenderer.h 
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
    src/ArgParser.h
    src/ArgParser.cc
    src/Base64Stream.h
    src/Base64Stream.cc
    src/Color.h
    src/Color.cc
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
target_link_libraries(pdf2htmlEX ${PDF2HTMLEX_LIBS})

install (TARGETS pdf2htmlEX DESTINATION bin)
file (GLOB datafiles 
    share/base.css
    share/fancy.css
    share/jquery.js
    share/pdf2htmlEX.js
    share/manifest
    )
install (FILES ${datafiles} DESTINATION share/pdf2htmlEX)
install (FILES pdf2htmlEX.1 DESTINATION share/man/man1)

enable_testing()
add_test(test_naming 
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    python test/test_naming.py)
