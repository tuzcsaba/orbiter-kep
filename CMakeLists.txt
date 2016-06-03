cmake_minimum_required (VERSION 2.6)

set (CMAKE_CXX_STANDARD 11)

project (OrbiterKEP C CXX)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/OrbiterKEPConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/OrbiterKEPConfig.h
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(/usr/local/include)
include_directories(/usr/local/include/mongocxx/v_noabi)
include_directories(/usr/local/include/bsoncxx/v_noabi)

link_directories(/usr/local/lib)
link_directories(/usr/lib)

include_directories(src)
include_directories(src/problems)
include_directories(third_party/gnuplot-iostream)

OPTION(BUILD_STATIC "Build the executable with static linkage" OFF)

OPTION(BUILD_MONGODB "Build support for MongoDB to persist solutons" ON)

add_subdirectory(src)

file(GLOB ORBITERKEP_SRC_LIST 
  "src/**/*.cpp"
  "src/*.cpp"
  "OrbiterKEP.cpp"
)

add_executable(OrbiterKEP ${ORBITERKEP_SRC_LIST})

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

set (PROTOBUF_IMPORT_DIRS
  proto
  )

set (ESA_LIBRARIES  
  libpagmo.a
  libkeplerian_toolbox.a
)

if (BUILD_MONGODB)
  set (MONGO_LIBRARIES
    libmongocxx.a
    libbsoncxx.a
  )
endif(BUILD_MONGODB)

ELSE(BUILD_STATIC)
set (BOOST_LIBRARIES
  boost_filesystem
  boost_system
  boost_serialization
  boost_iostreams
)

set (ESA_LIBRARIES  
  pagmo
  keplerian_toolbox
)

if (BUILD_MONGODB)
  set (MONGO_LIBRARIES
    mongocxx
    bsoncxx
  )
endif(BUILD_MONGODB)

ENDIF(BUILD_STATIC)

ADD_SUBDIRECTORY(proto)
target_link_libraries(OrbiterKEP proto ${PROTOBUF_LIBRARY} ${MONGO_LIBRARIES} ${ESA_LIBRARIES} ${BOOST_LIBRARIES})

INSTALL(TARGETS OrbiterKEP RUNTIME DESTINATION bin)

