cmake_minimum_required (VERSION 3.1)
project(orbiter-kep)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

configure_file(
  ${CMAKE_SOURCE_DIR}/config/OrbiterKEPConfig.h.in
  ${CMAKE_BINARY_DIR}/config/OrbiterKEPConfig.h
)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/config)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

OPTION(BUILD_STATIC "Build the executable with static linkage" OFF)

OPTION(BUILD_MONGODB "Build support for MongoDB to persist solutons" OFF)
OPTION(BUILD_PLOT "Build plotting tools for visualizing" OFF)

IF(BUILD_PLOT)
SET(gnuplot-iostream_INCLUDES ${CMAKE_SOURCE_DIR}/third_party/gnuplot-iostream)
ENDIF()

ADD_SUBDIRECTORY(orbiterkep-lib)
ADD_SUBDIRECTORY(OrbiterKEP)
