project(pdftohtmlEX)
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

find_package(PkgConfig)
pkg_check_modules(POPPLER REQUIRED poppler)
include_directories(${POPPLER_INCLUDE_DIRS})
link_directories ( ${POPPLER_LIBRARY_DIRS} )
find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
link_directories ( ${Boost_LIBRARY_DIRS} )

set(PDFTOHTMLEX_VERSION "0.1")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wunused-function")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

add_executable(pdftohtmlEX src/pdftohtmlEX.cc src/HTMLRenderer.cc src/HTMLRenderer.h src/BackgroundRenderer.cc src/BackgroundRenderer.h src/Consts.h)
target_link_libraries(pdftohtmlEX poppler boost_program_options)


