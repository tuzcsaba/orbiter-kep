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
unset(libbson_LIBRARIES CACHE)
unset(libbson_INCLUDE_DIRS CACHE)
unset(libbson_LIBRARY CACHE)
unset(libbson_INCLUDE_DIR CACHE)
unset(libbson_LIBNAME)


SET(libbson_ROOT ${CMAKE_INSTALL_PREFIX} STRING "Manual search path for libbson")

set(_libbson_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

set(libbson_LIBNAME "bson-1.0" STRING "Mangoc Library name")

if (WIN32)
  if (libbson_USE_STATIC_LIBS)
    set(libbson_LIBNAME "bson-static-1.0" STRING "Mangoc Library name")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES "_dll.lib")
  endif()
else()
  if (libbson_USE_STATIC_LIBS)
    set(libbson_LIBNAME "bson-static-1.0" STRING "Mangoc library name")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.dylib")
  endif()
endif()

find_path(libbson_INCLUDE_DIR NAMES bson.h
  HINTS ${libbson_ROOT}/include
  PATH_SUFFIXES libbson-1.0)

include(FindPackageHandleStandardArgs)

find_library(libbson_LIBRARY ${libbson_LIBNAME}
  PATHS ${libbson_ROOT}
  PATH_SUFFIXES lib
  NO_DEFAULT_PATH)

set(CMAKE_FIND_LIBRARY_SUFFIXES ${_libbson_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

find_package_handle_standard_args(libbson DEFAULT_MSG
  libbson_INCLUDE_DIR libbson_LIBRARY libbson_LIBNAME)

if(libbson_FOUND)
  set(libbson_INCLUDE_DIRS ${libbson_INCLUDE_DIR})
  set(libbson_LIBRARIES ${libbson_LIBRARY})
endif()


