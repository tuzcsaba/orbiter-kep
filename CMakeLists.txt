cmake_minimum_required (VERSION 2.6)

set (CMAKE_CXX_STANDARD 11)

project (OrbiterKEP C CXX)

include_directories(/usr/local/include)
include_directories(src)
include_directories(src/problems)
include_directories(third_party/gnuplot-iostream)

OPTION(BUILD_STATIC "Build the executable with static linkage" OFF)

add_subdirectory(src)

add_executable(OrbiterKEP OrbiterKEP.cpp)

# The version number
set (OrbiterKEP_VERSION_MAJOR 0)
set (OrbiterKEP_VERSION_MINOR 1)

if(BUILD_STATIC)
set (BOOST_LIBRARIES
  libboost_filesystem.a
  libboost_system.a
  libboost_serialization.a
  libboost_iostreams.a
)
target_link_libraries(OrbiterKEP libpagmo.a libkeplerian_toolbox.a orbiterkep_static ${BOOST_LIBRARIES})
ELSE(BUILD_STATIC)
set (BOOST_LIBRARIES
  boost_filesystem
  boost_system
  boost_serialization
  boost_iostreams
)
target_link_libraries(OrbiterKEP pagmo keplerian_toolbox orbiterkep_static ${BOOST_LIBRARIES})
ENDIF(BUILD_STATIC)

INSTALL(TARGETS OrbiterKEP RUNTIME DESTINATION bin)

