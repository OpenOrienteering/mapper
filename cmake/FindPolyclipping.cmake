#.rst:
# FindPolyclipping
# --------
#
# Find the polyclipping includes and library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``Polyclipping::Polyclipping``,
# if Polyclipping has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   POLYCLIPPING_INCLUDE_DIRS   - where to find clipper.hpp, etc.
#   POLYCLIPPING_LIBRARIES      - List of libraries when using libpolyclipping.
#   POLYCLIPPING_FOUND          - True if libpolyclipping found.
#
# ::
#
#   POLYCLIPPING_VERSION        - The version of libpolyclipping found (x.y.z)
#   POLYCLIPPING_VERSION_MAJOR  - The major version of libpolyclipping
#   POLYCLIPPING_VERSION_MINOR  - The minor version of libpolyclipping
#   POLYCLIPPING_VERSION_PATCH  - The patch version of libpolyclipping
#   POLYCLIPPING_VERSION_TWEAK  - The tweak version of libpolyclipping
#   POLYCLIPPING_VERSION_COUNT  - The number of version components
#
# Hints
# ^^^^^
#
# A user may set ``POLYCLIPPING_ROOT`` to a libpolyclipping installation root
# to tell this module where to look exclusively.

#=============================================================================
# Copyright 2016 Kai Pastor
#
#
# This file was derived from CMake 3.5's module FindZLIB.cmake
# which has the following terms:
#
# Copyright 2001-2011 Kitware, Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * The names of Kitware, Inc., the Insight Consortium, or the names of
#   any consortium members, or of any contributors, may not be used to
#   endorse or promote products derived from this software without
#   specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

# Search POLYCLIPPING_ROOT exclusively if it is set.
if(POLYCLIPPING_ROOT)
  set(_POLYCLIPPING_SEARCH PATHS ${POLYCLIPPING_ROOT} NO_DEFAULT_PATH)
else()
  set(_POLYCLIPPING_SEARCH)
endif()

find_path(POLYCLIPPING_INCLUDE_DIR NAMES clipper.hpp ${_POLYCLIPPING_SEARCH} PATH_SUFFIXES include include/polyclipping)
mark_as_advanced(POLYCLIPPING_INCLUDE_DIR)

# Allow POLYCLIPPING_LIBRARY to be set manually, as the location of the polyclipping library
if(NOT POLYCLIPPING_LIBRARY)
  set(POLYCLIPPING_NAMES polyclipping)
  set(POLYCLIPPING_NAMES_DEBUG polyclippingd)
  find_library(POLYCLIPPING_LIBRARY_RELEASE NAMES ${POLYCLIPPING_NAMES} ${_POLYCLIPPING_SEARCH} PATH_SUFFIXES lib)
  find_library(POLYCLIPPING_LIBRARY_DEBUG NAMES ${POLYCLIPPING_NAMES_DEBUG} ${_POLYCLIPPING_SEARCH} PATH_SUFFIXES lib)
  include(SelectLibraryConfigurations)
  select_library_configurations(POLYCLIPPING)
endif()

if(POLYCLIPPING_INCLUDE_DIR AND EXISTS "${POLYCLIPPING_INCLUDE_DIR}/clipper.hpp")
    file(STRINGS "${POLYCLIPPING_INCLUDE_DIR}/clipper.hpp" POLYCLIPPING_H REGEX "^#define CLIPPER_VERSION \"[^\"]*\"$")

    string(REGEX REPLACE "^.*CLIPPER_VERSION \"([0-9]+).*$" "\\1" POLYCLIPPING_VERSION_MAJOR "${POLYCLIPPING_H}")
    string(REGEX REPLACE "^.*CLIPPER_VERSION \"[0-9]+\\.([0-9]+).*$" "\\1" POLYCLIPPING_VERSION_MINOR  "${POLYCLIPPING_H}")
    string(REGEX REPLACE "^.*CLIPPER_VERSION \"[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" POLYCLIPPING_VERSION_PATCH "${POLYCLIPPING_H}")
    set(POLYCLIPPING_VERSION "${POLYCLIPPING_VERSION_MAJOR}.${POLYCLIPPING_VERSION_MINOR}.${POLYCLIPPING_VERSION_PATCH}")
    set(POLYCLIPPING_VERSION_COUNT 3)

    # only append a TWEAK version if it exists:
    set(POLYCLIPPING_VERSION_TWEAK 0)
    if( "${POLYCLIPPING_H}" MATCHES "CLIPPER_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+)")
        set(POLYCLIPPING_VERSION_TWEAK "${CMAKE_MATCH_1}")
        set(POLYCLIPPING_VERSION "${POLYCLIPPING_VERSION}.${POLYCLIPPING_VERSION_TWEAK}")
        set(POLYCLIPPING_VERSION_COUNT 4)
    endif()
endif()

# handle the QUIETLY and REQUIRED arguments and set POLYCLIPPING_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Polyclipping
  REQUIRED_VARS
    POLYCLIPPING_INCLUDE_DIR
    POLYCLIPPING_LIBRARY
  VERSION_VAR
    POLYCLIPPING_VERSION
)

if(POLYCLIPPING_FOUND)
    set(POLYCLIPPING_INCLUDE_DIRS ${POLYCLIPPING_INCLUDE_DIR})

    if(NOT POLYCLIPPING_LIBRARIES)
      set(POLYCLIPPING_LIBRARIES ${POLYCLIPPING_LIBRARY})
    endif()

    if(NOT TARGET Polyclipping::Polyclipping)
      add_library(Polyclipping::Polyclipping UNKNOWN IMPORTED)
      set_target_properties(Polyclipping::Polyclipping PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${POLYCLIPPING_INCLUDE_DIRS}")

      if(POLYCLIPPING_LIBRARY_RELEASE)
        set_property(TARGET Polyclipping::Polyclipping APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(Polyclipping::Polyclipping PROPERTIES
          IMPORTED_LOCATION_RELEASE "${POLYCLIPPING_LIBRARY_RELEASE}")
      endif()

      if(POLYCLIPPING_LIBRARY_DEBUG)
        set_property(TARGET Polyclipping::Polyclipping APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(Polyclipping::Polyclipping PROPERTIES
          IMPORTED_LOCATION_DEBUG "${POLYCLIPPING_LIBRARY_DEBUG}")
      endif()

      if(NOT POLYCLIPPING_LIBRARY_RELEASE AND NOT POLYCLIPPING_LIBRARY_DEBUG)
        set_property(TARGET Polyclipping::Polyclipping APPEND PROPERTY
          IMPORTED_LOCATION "${POLYCLIPPING_LIBRARY}")
      endif()
    endif()
endif()
