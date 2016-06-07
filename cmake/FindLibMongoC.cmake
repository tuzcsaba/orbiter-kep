# Copyright 2016 MongoDB Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Find libmongo-c, either via pkg-config, find-package in config mode,
# or other less admirable jiggery-pokery
unset(libmongoc_LIBRARIES CACHE)
unset(libmongoc_INCLUDE_DIRS CACHE)
unset(libmongoc_LIBRARY CACHE)
unset(libmongoc_INCLUDE_DIR CACHE)
unset(libmongoc_LIBNAME)


SET(LIBMONGOC_DIR ${CMAKE_INSTALL_PREFIX} CACHE STRING "Manual search path for libmongoc")

set(_libmongoc_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

set(libmongoc_LIBNAME "mongoc-1.0" STRING "Mangoc Library name")

if (WIN32)
  if (libmongoc_USE_STATIC_LIBS)
    set(libmongoc_LIBNAME "mongoc-static-1.0" STRING "Mangoc Library name")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES "_dll.lib;.dll")
  endif()
else()
  if (libmongoc_USE_STATIC_LIBS)
    set(libmongoc_LIBNAME "mongoc-static-1.0" STRING "Mangoc library name")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.dylib")
  endif()
endif()

find_path(libmongoc_INCLUDE_DIR NAMES mongoc.h
  HINTS ${LIBMONGOC_DIR}
  PATH_SUFFIXES libmongoc-1.0 src/mongoc)

include(FindPackageHandleStandardArgs)

find_library(libmongoc_LIBRARY ${libmongoc_LIBNAME}
  PATHS ${LIBMONGOC_DIR}
  PATH_SUFFIXES lib bin
  NO_DEFAULT_PATH)

set(CMAKE_FIND_LIBRARY_SUFFIXES ${_libmongoc_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

find_package_handle_standard_args(libmongoc DEFAULT_MSG
  libmongoc_INCLUDE_DIR libmongoc_LIBRARY libmongoc_LIBNAME)

if(libmongoc_FOUND)
  set(libmongoc_INCLUDE_DIRS ${libmongoc_INCLUDE_DIR})
  set(libmongoc_LIBRARIES ${libmongoc_LIBRARY})
endif()


