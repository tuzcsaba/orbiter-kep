cmake_minimum_required (VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

project (OrbiterKEP C CXX)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/OrbiterKEPConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/OrbiterKEPConfig.h
)

SET(CMAKE_INCLUDE_PATH ${CMAKE_INSTALL_PREFIX}/include)
SET(CMAKE_LIBRARY_PATH ${CMAKE_INSTALL_PREFIX}/lib)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(${CMAKE_INSTALL_PREFIX}/include)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)
link_directories(/usr/lib)

include_directories(src)
include_directories(src/problems)
include_directories(third_party/gnuplot-iostream)

OPTION(BUILD_STATIC "Build the executable with static linkage" OFF)

OPTION(BUILD_MONGODB "Build support for MongoDB to persist solutons" ON)

SET(CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

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

SET(Boost_USE_STATIC_LIBS ${BUILD_STATIC})
set(Boost_USE_MULTITHREADED ON)
SET(Boost_USE_STATIC_RUNTIME ${BUILD_STATIC})
find_Package(Boost COMPONENTS filesystem system serialization iostreams)

if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

if (BUILD_MONGODB)

  set(LIBMONGOC_USE_STATIC_LIBS ${BUILD_STATIC})
  set(LIBBSON_USE_STATIC_LIBS ${BUILD_STATIC})

  find_package(LIBMONGOC REQUIRED)
  find_package(LIBBSON REQUIRED)
 
  include_directories(${LIBMONGOC_INCLUDE_DIRS})
  include_directories(${LIBBSON_INCLUDE_DIRS})

  link_directories(${LIBMONGOC_LIBRARY_DIRS})
  link_directories(${LIBBSON_LIBRARY_DIRS})

  target_link_libraries(OrbiterKEP ${LIBMONGOC_LIBRARIES} ${LIBBSON_LIBRARY})

endif(BUILD_MONGODB)


if(BUILD_STATIC)

set (ESA_LIBRARIES  
  libpagmo.a
  libkeplerian_toolbox.a
)

ELSE(BUILD_STATIC)

set (ESA_LIBRARIES  
  pagmo
  keplerian_toolbox
)

ENDIF(BUILD_STATIC)

ADD_SUBDIRECTORY(proto)
target_link_libraries(OrbiterKEP proto ${PROTOBUF_LIBRARY} ${MONGO_LIBRARIES} ${ESA_LIBRARIES} ${Boost_LIBRARIES})

INSTALL(TARGETS OrbiterKEP RUNTIME DESTINATION bin)

