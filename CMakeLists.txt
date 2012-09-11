project(pdftohtmlEX)
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

include_directories(${CMAKE_SOURCE_DIR}/src/include)

find_package(PkgConfig)
pkg_check_modules(POPPLER REQUIRED poppler>=0.20.0)
include_directories(${POPPLER_INCLUDE_DIRS})
link_directories(${POPPLER_LIBRARY_DIRS})

find_path(FF_INCLUDE_PATH fontforge/fontforge.h)
if(FF_INCLUDE_PATH)
    message("Found fontforge.h: ${FF_INCLUDE_PATH}/fontforge/fontforge.h")
    include_directories(${FF_INCLUDE_PATH}/fontforge)
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

find_path(FF_CONFIG_INCLUDE_PATH fontforge/config.h)
if(FF_CONFIG_INCLUDE_PATH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${FF_INCLUDE_PATH}/fontforge/config.h")
endif()

foreach(FF_LIB_NAME 
${CMAKE_IMPORT_LIBRARY_PREFIX}fontforge${CMAKE_IMPORT_LIBRARY_SUFFIX}
${CMAKE_SHARED_LIBRARY_PREFIX}fontforge${CMAKE_SHARED_LIBRARY_SUFFIX}
${CMAKE_STATIC_LIBRARY_PREFIX}fontforge${CMAKE_STATIC_LIBRARY_SUFFIX}
)
find_library(FF_LIB ${FF_LIB_NAME})
if(FF_LIB)
    message("Found ${FF_LIB_NAME}: ${FF_LIB}")
    set(FONTFORGE_LIBRARIES ${FF_LIB})
    break()
endif()
endforeach()

if(NOT FF_LIB)
    set(FONTFORGE_LIBRARIES fontforge)
    message("Error: cannot locate ${FF_LIB_NAME}")
endif()

message("Looking for libraries of python, which is required by fontforge, if you can link fontforge without python, you may disable this")
pkg_search_module(PYTHON python python-2.7)

set(PDF2HTMLEX_VERSION "0.3")
set(ARCHIVE_NAME pdf2htmlex-${PDF2HTMLEX_VERSION})
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wunused-function")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wunused-function")
# clang compiler need c++11 flag
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

# CYGWIN bug
if(CYGWIN)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=gnu++0x")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x")
endif()

configure_file (${CMAKE_SOURCE_DIR}/src/include/pdf2htmlEX-config.h.in ${CMAKE_SOURCE_DIR}/src/include/pdf2htmlEX-config.h)

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

target_link_libraries(pdf2htmlEX ${POPPLER_LIBRARIES} ${FONTFORGE_LIBRARIES} ${PYTHON_LIBRARIES})

install (TARGETS pdf2htmlEX DESTINATION bin)
file (GLOB datafiles share/*)
install (FILES ${datafiles} DESTINATION share/pdf2htmlEX)
install (FILES pdf2htmlEX.1 DESTINATION share/man/man1)
