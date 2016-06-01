cmake_minimum_required (VERSION 2.6)

set (CMAKE_CXX_STANDARD 11)

project (OrbiterKEP C CXX)

include_directories(/usr/local/include)
include_directories(src)
include_directories(src/problems)
include_directories(third_party/gnuplot-iostream)

add_subdirectory(src)

add_executable(OrbiterKEP OrbiterKEP.cpp)

# The version number
set (OrbiterKEP_VERSION_MAJOR 0)
set (OrbiterKEP_VERSION_MINOR 1)

set (BOOST_LIBRARIES
  boost_system
  boost_serialization
  boost_iostreams
)

target_link_libraries(OrbiterKEP pagmo keplerian_toolbox orbiterkep_static ${BOOST_LIBRARIES})
