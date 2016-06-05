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
unset(pagmo_LIBRARIES CACHE)
unset(pagmo_INCLUDE_DIRS CACHE)
unset(pagmo_LIBRARY CACHE)
unset(pagmo_INCLUDE_DIR CACHE)
unset(pagmo_LIBNAME)


SET(pagmo_ROOT ${CMAKE_INSTALL_PREFIX} STRING "Manual search path for pagmo")

set(_pagmo_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

set(pagmo_LIBNAME "pagmo" STRING "Mangoc Library name")

if (WIN32)
  if (pagmo_USE_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES "_dll.lib")
  endif()
else()
  if (pagmo_USE_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.dylib")
  endif()
endif()

find_path(pagmo_INCLUDE_DIR NAMES pagmo.h
  HINTS ${pagmo_ROOT}/include
  PATH_SUFFIXES pagmo)

include(FindPackageHandleStandardArgs)

find_library(pagmo_LIBRARY ${pagmo_LIBNAME}
  PATHS ${pagmo_ROOT}
  PATH_SUFFIXES lib
  NO_DEFAULT_PATH)

set(CMAKE_FIND_LIBRARY_SUFFIXES ${_pagmo_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

find_package_handle_standard_args(pagmo DEFAULT_MSG
  pagmo_INCLUDE_DIR pagmo_LIBRARY pagmo_LIBNAME)

if(pagmo_FOUND)
  set(pagmo_INCLUDE_DIRS ${pagmo_INCLUDE_DIR})
  set(pagmo_LIBRARIES ${pagmo_LIBRARY})
endif()


