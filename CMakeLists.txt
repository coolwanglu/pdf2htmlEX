project(pdftohtmlEX)
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

include_directories(${CMAKE_SOURCE_DIR}/src/include)

set(PDF2HTMLEX_VERSION "0.4")
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

# fontforge starts using pkg-config 'correctly' since 2.0.0
pkg_check_modules(FONTFORGE libfontforge>=2.0.0)
if(FONTFORGE_FOUND)
    include_directories(${FONTFORGE_INCLUDE_DIRS})
    link_directories(${FONTFORGE_LIBRARY_DIRS})
    set(PDF2HTMLEX_LIBS ${PDF2HTMLEX_LIBS} ${FONTFORGE_LIBRARIES})
else()
    message("Trying to locate fontforge...")
    find_path(FF_INCLUDE_PATH fontforge/fontforge.h)
    if(FF_INCLUDE_PATH)
        message("Found fontforge.h: ${FF_INCLUDE_PATH}/fontforge/fontforge.h")
        set(FONTFORGE_INCLUDE_DIRS ${FF_INCLUDE_PATH}/fontforge)
        include_directories(${FONTFORGE_INCLUDE_DIRS})
        # MacOSX gettext is in /opt/local/include - strange
        if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")  
          find_path(GETTEXT_INCLUDE_PATH libintl.h)
          if(GETTEXT_INCLUDE_PATH)
            include_directories(${GETTEXT_INCLUDE_PATH})
          else()
            # disable i18n if there is no gettext
            set(NO_GETTEXT "YesPlease")
          endif()
        endif()
    else()
        message(FATAL_ERROR "Error: cannot locate fontforge.h")
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

find_path(FF_CONFIG_INCLUDE_PATH config.h PATHS
  ${FONTFORGE_INCLUDE_DIRS} NO_DEFAULT_PATH)
if(FF_CONFIG_INCLUDE_PATH)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${FF_CONFIG_INCLUDE_PATH}/config.h")
message("Found config.h: ${FF_CONFIG_INCLUDE_PATH}/config.h")
else()
message("Cannot locate config.h for fontforge")
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wunused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wunused-function")
# clang compiler need c++11 flag
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

# CYGWIN bug
if(CYGWIN)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=gnu++0x")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x")
endif()

configure_file (${CMAKE_SOURCE_DIR}/src/include/pdf2htmlEX-config.h.in ${CMAKE_SOURCE_DIR}/src/include/pdf2htmlEX-config.h)
configure_file (${CMAKE_SOURCE_DIR}/pdf2htmlEX.1.in ${CMAKE_SOURCE_DIR}/pdf2htmlEX.1)

add_executable(pdf2htmlEX 
    src/pdf2htmlEX.cc 
    src/include/HTMLRenderer.h 
    src/HTMLRenderer/general.cc
    src/HTMLRenderer/state.cc
    src/HTMLRenderer/install.cc
    src/HTMLRenderer/export.cc
    src/HTMLRenderer/text.cc
    src/HTMLRenderer/image.cc
    src/include/namespace.h
    src/HTMLRenderer/LineBuffer.cc
    src/include/ff.h
    src/ff.c
    src/include/BackgroundRenderer.h 
    src/BackgroundRenderer.cc 
    src/include/FontPreprocessor.h
    src/FontPreprocessor.cc
    src/include/util.h
    src/util.cc
    src/include/ArgParser.h
    src/ArgParser.cc
    src/include/pdf2htmlEX-config.h
    )
target_link_libraries(pdf2htmlEX ${PDF2HTMLEX_LIBS})

install (TARGETS pdf2htmlEX DESTINATION bin)
file (GLOB datafiles share/*)
install (FILES ${datafiles} DESTINATION share/pdf2htmlEX)
install (FILES pdf2htmlEX.1 DESTINATION share/man/man1)
