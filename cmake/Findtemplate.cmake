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
unset($$package$$_LIBRARY)
unset($$package$$_INCLUDE_DIR)
unset($$package$$_LIBNAME)


SET($$package$$_ROOT ${CMAKE_INSTALL_PREFIX} CACHE STRING "Manual search path for $$package$$")

set(_$$package$$_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

set($$package$$_LIBNAME "mongoc-1.0" STRING "Mangoc Library name")

if (WIN32)
  if ($$package$$_USE_STATIC_LIBS)
    set($$package$$_LIBNAME "mongoc-static-1.0" STRING "Mangoc Library name")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES "_dll.lib")
  endif()
else()
  if ($$package$$_USE_STATIC_LIBS)
    set($$package$$_LIBNAME "mongoc-static-1.0" STRING "Mangoc library name")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.dylib")
  endif()
endif()

find_path($$package$$_INCLUDE_DIR NAMES mongoc.h
  HINTS ${$$package$$_ROOT}/include
  PATH_SUFFIXES lib${$$package$$_LIBNAME})

include(FindPackageHandleStandardArgs)

find_library($$package$$_LIBRARY ${$$package$$_LIBNAME}
  PATHS ${$$package$$_ROOT}
  PATH_SUFFIXES lib
  NO_DEFAULT_PATH)

set(CMAKE_FIND_LIBRARY_SUFFIXES ${_$$package$$_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

find_package_handle_standard_args($$package$$ DEFAULT_MSG
  $$package$$_INCLUDE_DIR $$package$$_LIBRARY $$package$$_LIBNAME)

if($$package$$_FOUND)
  set($$package$$_INCLUDE_DIRS ${$$package$$_INCLUDE_DIR})
  set($$package$$_LIBRARIES ${$$package$$_LIBRARY})
endif()


