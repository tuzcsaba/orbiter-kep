cmake_minimum_required (VERSION 3.1)
project(orbiter-kep)

configure_file(
  ${CMAKE_SOURCE_DIR}/config/OrbiterKEPConfig.h.in
  ${CMAKE_BINARY_DIR}/config/OrbiterKEPConfig.h
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/config)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

OPTION(BUILD_STATIC "Build the executable with static linkage" OFF)
OPTION(BUILD_MONGODB "Build support for MongoDB to persist solutons" ON)


SET(gnuplot-iostream_INCLUDES ${CMAKE_SOURCE_DIR}/third_party/gnuplot-iostream)

ADD_SUBDIRECTORY(orbiterkep-lib)
ADD_SUBDIRECTORY(OrbiterKEP)
