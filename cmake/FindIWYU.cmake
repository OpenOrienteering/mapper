#.rst:
# FindIWYU
# --------
#
# Find include-what-you-use (iwyu).
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``IWYU::iwyu``
# if the include-what-you-use executable has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   IWYU_EXECUTABLE     - where to find include-what-you-use (iwyu)
#   IWYU_FOUND          - True if iwyu found.
#
# ::
#
#   IWYU_VERSION        - The version of iwyu found (x.y)
#   IWYU_VERSION_MAJOR  - The major version of iwyu
#   IWYU_VERSION_MINOR  - The minor version of iwyu
#   IWYU_VERSION_PATCH  - always 0
#   IWYU_VERSION_TWEAK  - always 0
#   IWYU_VERSION_COUNT  - The number of version components, always 2
#
# Hints
# ^^^^^
#
# A user may set ``IWYU_ROOT_DIR`` to an iwyu installation root to tell this
# module where to look exclusively, or he may set the ``IWYU_EXECUTABLE``
# variable directly.

#=============================================================================
# Copyright 2019 Kai Pastor
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

# Search IWYU_ROOT_DIR exclusively if it is set.
if(IWYU_ROOT_DIR)
	set(iwyu_search PATHS ${IWYU_ROOT_DIR} NO_DEFAULT_PATH)
else()
	set(iwyu_search)
endif()

find_program(IWYU_EXECUTABLE
  NAMES
    include-what-you-use
    iwyu
  ${iwyu_search}
  PATH_SUFFIXES bin
  DOC "The filepath of the include-what-you-use executable"
)

if(IWYU_EXECUTABLE)
	execute_process(
	  COMMAND "${IWYU_EXECUTABLE}" --version
	  OUTPUT_VARIABLE iwyu_version_string
	)
	string(REGEX MATCH "include-what-you-use ([0-9]+)[.]([0-9]+)" ignored "${iwyu_version_string}")
	set(IWYU_VERSION_MAJOR "${CMAKE_MATCH_1}")
	set(IWYU_VERSION_MINOR "${CMAKE_MATCH_2}")
	set(IWYU_VERSION "${IWYU_VERSION_MAJOR}.${IWYU_VERSION_MINOR}")
	set(IWYU_VERSION_COUNT 2)
endif()

# handle the QUIETLY and REQUIRED arguments and set IWYU_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IWYU
  REQUIRED_VARS
    IWYU_EXECUTABLE
  VERSION_VAR
    IWYU_VERSION
)

if(IWYU_FOUND)
	if(NOT TARGET IWYU::iwyu)
		add_executable(IWYU::iwyu IMPORTED)
		set_target_properties(IWYU::iwyu PROPERTIES
		  IMPORTED_LOCATION "${IWYU_EXECUTABLE}")
	endif()
endif()
