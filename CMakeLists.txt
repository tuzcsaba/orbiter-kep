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

SET(BUILD_SHARED_LIBRARIES ${BUILD_STATIC})

add_executable(OrbiterKEP ${ORBITERKEP_SRC_LIST})

# The version number
set (OrbiterKEP_VERSION_MAJOR 0)
set (OrbiterKEP_VERSION_MINOR 1)


SET(Boost_USE_STATIC_LIBS ${BUILD_STATIC})
set(Boost_USE_MULTITHREADED ON)
SET(Boost_USE_STATIC_RUNTIME ${BUILD_STATIC})
find_package(Boost COMPONENTS filesystem system serialization iostreams thread)

if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
  target_link_libraries(OrbiterKEP ${Boost_LIBRARIES})
  target_compile_options(OrbiterKEP PUBLIC ${Boost_CFLAGS_OTHER})
endif()

if (BUILD_MONGODB)
  unset(libmongoc_LIBRARIES)
  unset(libmongoc_INCLUDE_DIRS)

  set(libmongoc_USE_STATIC_LIBS ${BUILD_STATIC})
  set(libbson_USE_STATIC_LIBS ${BUILD_STATIC})

  find_package(libmongoc REQUIRED)
  find_package(libbson REQUIRED)
 
  include_directories(${libmongoc_INCLUDE_DIRS})
  link_directories(${libmongoc_LIBRARY_DIRS})
  target_link_libraries(OrbiterKEP ${libmongoc_LIBRARIES})
  target_compile_options(OrbiterKEP PUBLIC ${libmongoc_CFLAGS_OTHER})

  include_directories(${libbson_INCLUDE_DIRS})
  link_directories(${libbson_LIBRARY_DIRS})
  target_link_libraries(OrbiterKEP ${libbson_LIBRARIES})
  target_compile_options(OrbiterKEP PUBLIC ${libbson_CFLAGS_OTHER})

endif(BUILD_MONGODB)


set(pagmo_USE_STATIC_LIBS ${BUILD_STATIC})
set(pykep_USE_STATIC_LIBS ${BUILD_STATIC})

find_package(pagmo REQUIRED)
find_package(pykep REQUIRED)

include_directories(${pagmo_INCLUDE_DIRS} ${pykep_INCLUDE_DIRS})
link_directories(${pagmo_LIBRARY_DIRS} ${pykep_LIBRARY_DIRS})
target_link_libraries(OrbiterKEP ${pagmo_LIBRARIES} ${pykep_LIBRARIES})
target_compile_options(OrbiterKEP PUBLIC ${pagmo_CFLAGS_OTHER} ${pykep_CFLAGS_OTHER})


ADD_SUBDIRECTORY(proto)
target_link_libraries(OrbiterKEP proto)

INSTALL(TARGETS OrbiterKEP RUNTIME DESTINATION bin)

