project(pdftohtmlEX)
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

find_package(PkgConfig)
pkg_check_modules(POPPLER REQUIRED poppler)
include_directories(${POPPLER_INCLUDE_DIRS})
link_directories ( ${POPPLER_LIBRARY_DIRS} )
find_package(Boost REQUIRED COMPONENTS program_options filesystem system)
include_directories(${Boost_INCLUDE_DIRS})
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories(src)

set(PDF2HTMLEX_VERSION "0.1")
set(ARCHIVE_NAME pdf2htmlex-${PDF2HTMLEX_VERSION})
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wunused-function")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

configure_file (src/config.h.in src/config.h)

add_executable(pdf2htmlEX 
    src/pdf2htmlEX.cc 
    src/HTMLRenderer.h 
    src/HTMLRenderer/general.cc
    src/HTMLRenderer/state.cc
    src/HTMLRenderer/install.cc
    src/HTMLRenderer/export.cc
    src/HTMLRenderer/text.cc
    src/HTMLRenderer/image.cc
    src/HTMLRenderer/namespace.h
    src/BackgroundRenderer.h 
    src/BackgroundRenderer.cc 
    src/Consts.h 
    src/Consts.cc 
    src/util.h
    src/config.h)

target_link_libraries(pdf2htmlEX poppler boost_program_options boost_filesystem boost_system)

install (TARGETS pdf2htmlEX DESTINATION bin)
file (GLOB datafiles share/*)
install (FILES ${datafiles} DESTINATION share/pdf2htmlEX)
