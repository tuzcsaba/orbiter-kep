CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(orbiterkep-lib C CXX)


SET (${PROJECT_NAME}_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
SET (${PROJECT_NAME}_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
SET (${PROJECT_NAME}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB orbiterkep-lib_SRCS
  "${${PROJECT_NAME}_SOURCE_DIR}/**/*.cpp"
  "${${PROJECT_NAME}_SOURCE_DIR}/*.cpp"
  "${${PROJECT_NAME}_SOURCE_DIR}/**/*.cc"
  "${${PROJECT_NAME}_SOURCE_DIR}/*.cc"
  "${${PROJECT_NAME}_SOURCE_DIR}/**/*.c"
  "${${PROJECT_NAME}_SOURCE_DIR}/*.c"
)

file(GLOB orbiterkep-lib_INCL
  "${${PROJECT_NAME}_INCLUDE_DIR}/**/*.h"
  "${${PROJECT_NAME}_INCLUDE_DIR}/**/*.hpp"
  "${${PROJECT_NAME}_INCLUDE_DIR}/*.h"
  "${${PROJECT_NAME}_INCLUDE_DIR}/*.hpp"
)

add_subdirectory(proto)

INCLUDE_DIRECTORIES(${orbiterkep-proto_INCLUDES})

INCLUDE_DIRECTORIES(${gnuplot-iostream_INCLUDES})

IF (BUILD_STATIC)
  add_library(${PROJECT_NAME} STATIC ${orbiterkep-lib_SRCS})
ELSE()
  add_library(${PROJECT_NAME} ${orbiterkep-lib_SRCS})
ENDIF()

INCLUDE_DIRECTORIES(${${PROJECT_NAME}_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${${PROJECT_NAME}_BIN_DIR})

set_target_properties(${PROJECT_NAME}
  PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${PROJECT_NAME} orbiterkep-proto)

SET(Boost_USE_STATIC_LIBS ${BUILD_STATIC})
set(Boost_USE_MULTITHREADED ON)
SET(Boost_USE_STATIC_RUNTIME ${BUILD_STATIC})
find_package(Boost COMPONENTS filesystem system serialization iostreams thread)

include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

# PyKEP/PaGMO

set(pagmo_USE_STATIC_LIBS ${BUILD_STATIC})
set(pykep_USE_STATIC_LIBS ${BUILD_STATIC})

find_package(pagmo REQUIRED)
find_package(pykep REQUIRED)

include_directories(${pagmo_INCLUDE_DIRS} ${pykep_INCLUDE_DIRS})
# link_directories(${pagmo_LIBRARY_DIRS} ${pykep_LIBRARY_DIRS})
target_link_libraries(orbiterkep-lib ${pagmo_LIBRARIES} ${pykep_LIBRARIES})
# target_compile_options(orbiterkep-lib PUBLIC ${pagmo_CFLAGS_OTHER} ${pykep_CFLAGS_OTHER})

# MongoDB

if (BUILD_MONGODB)
  set(libmongoc_USE_STATIC_LIBS ${BUILD_STATIC})
  set(libbson_USE_STATIC_LIBS ${BUILD_STATIC})

  find_package(libmongoc REQUIRED)
  find_package(libbson REQUIRED)
 
  include_directories(${libmongoc_INCLUDE_DIRS})
#  link_directories(${libmongoc_LIBRARY_DIRS})
  target_link_libraries(orbiterkep-lib ${libmongoc_LIBRARIES})
#  target_compile_options(orbiterkep-lib PUBLIC ${libmongoc_CFLAGS_OTHER})

  include_directories(${libbson_INCLUDE_DIRS})
#  link_directories(${libbson_LIBRARY_DIRS})
  target_link_libraries(orbiterkep-lib ${libbson_LIBRARIES})
#  target_compile_options(orbiterkep-lib PUBLIC ${libbson_CFLAGS_OTHER})

endif(BUILD_MONGODB)


SET(${PROJECT_NAME}_INCLUDES ${${PROJECT_NAME}_INCLUDE_DIR}
                             ${${PROJECT_NAME}_BIN_DIR}
                             ${orbiterkep-proto_INCLUDES}
                             ${pagmo_INCLUDE_DIRS}
                             ${pykep_INCLUDE_DIRS}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
)

