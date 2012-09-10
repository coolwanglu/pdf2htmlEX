project(pdftohtmlEX)
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

include_directories(${CMAKE_SOURCE_DIR}/src/include)

find_package(PkgConfig)
pkg_check_modules(POPPLER REQUIRED poppler>=0.20.0)
include_directories(${POPPLER_INCLUDE_DIRS})
link_directories(${POPPLER_LIBRARY_DIRS})

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

find_path(FF_INCLUDE_PATH fontforge/fontforge.h)
if(FF_INCLUDE_PATH)
    message("Found fontforge.h: ${FF_INCLUDE_PATH}/fontforge/fontforge.h")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${FF_INCLUDE_PATH}/fontforge/config.h")
    include_directories(${FF_INCLUDE_PATH}/fontforge)
else()
    message(FATAL_ERROR "Error: cannot locate fontforge.h")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(FF_LIB_EXT ".so")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(FF_LIB_EXT ".dylib")
else()
    message("Set FF_LIB_EXT for your platform!")
endif()

find_library(FF_LIB libfontforge${FF_LIB_EXT})
if(FF_LIB)
    message("Found libfontforge${FF_LIB_EXT}: ${FF_LIB}")
    set(FONTFORGE_LIBRARIES ${FF_LIB})
else()
    message(FATAL_ERROR "Error: cannot locate libfontforge${FF_LIB_EXT}")
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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

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
    src/include/pdf2htmlEX-config.h
    )

target_link_libraries(pdf2htmlEX ${POPPLER_LIBRARIES} ${Boost_LIBRARIES} ${FONTFORGE_LIBRARIES} ${PYTHON_LIBRARIES})

install (TARGETS pdf2htmlEX DESTINATION bin)
file (GLOB datafiles share/*)
install (FILES ${datafiles} DESTINATION share/pdf2htmlEX)
install (FILES pdf2htmlEX.1 DESTINATION share/man/man1)
