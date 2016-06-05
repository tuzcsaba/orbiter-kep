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
unset(pykep_LIBRARIES CACHE)
unset(pykep_INCLUDE_DIRS CACHE)
unset(pykep_LIBRARY CACHE)
unset(pykep_INCLUDE_DIR CACHE)
unset(pykep_LIBNAME)
unset(spice_LIBRARY CACHE)


SET(pykep_ROOT ${CMAKE_INSTALL_PREFIX} STRING "Manual search path for pykep")

set(_pykep_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

set(pykep_LIBNAME "keplerian_toolbox" STRING "Mangoc Library name")

if (WIN32)
  if (pykep_USE_STATIC_LIBS)
    set(pykep_LIBNAME "keplerian_toolbox" STRING "Mangoc Library name")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES "_dll.lib")
  endif()
else()
  if (pykep_USE_STATIC_LIBS)
    set(pykep_LIBNAME "keplerian_toolbox" STRING "Mangoc library name")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.dylib")
  endif()
endif()

find_path(pykep_INCLUDE_DIR NAMES epoch.h
  HINTS ${pykep_ROOT}/include
  PATH_SUFFIXES keplerian_toolbox)

include(FindPackageHandleStandardArgs)

find_library(pykep_LIBRARY NAMES ${pykep_LIBNAME} 
  PATHS ${pykep_ROOT}
  PATH_SUFFIXES lib
  NO_DEFAULT_PATH)

if (pykep_USE_STATIC_LIBS)
  if (WIN32)
    set(spice_LIBNAME "cspice.lib")
  else()
    set(spice_LIBNAME "cspice.a")
  endif()

  find_library(spice_LIBRARY NAMES ${spice_LIBNAME}
    PATHS ${pykep_ROOT}
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH)

  if (spice_FOUND)
  endif()
endif()


set(CMAKE_FIND_LIBRARY_SUFFIXES ${_pykep_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

find_package_handle_standard_args(pykep DEFAULT_MSG
  pykep_INCLUDE_DIR pykep_LIBRARY pykep_LIBNAME)

if(pykep_FOUND)
  set(pykep_INCLUDE_DIRS ${pykep_INCLUDE_DIR})
  set(pykep_LIBRARIES ${pykep_LIBRARY} ${spice_LIBRARY})
endif()


