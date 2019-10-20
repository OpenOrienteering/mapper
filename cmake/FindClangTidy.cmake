#.rst:
# FindClangTidy
# -------------
#
# Find clang-tidy.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``Clang::Tidy``
# if the clang-tidy executable has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   ClangTidy_EXECUTABLE     - where to find clang-tidy
#   ClangTidy_FOUND          - True if clang-tidy found.
#
# ::
#
#   ClangTidy_VERSION        - The version of clang-tidy found (x.y.z)
#   ClangTidy_VERSION_MAJOR  - The major version of clang-tidy
#   ClangTidy_VERSION_MINOR  - The minor version of clang-tidy
#   ClangTidy_VERSION_PATCH  - The patch version of clang-tidy
#   ClangTidy_VERSION_TWEAK  - always 0
#   ClangTidy_VERSION_COUNT  - The number of version components, always 3
#
# Hints
# ^^^^^
#
# A user may set ``ClangTidy_ROOT_DIR`` to an clang-tidy installation root to
# tell this module where to look exclusively, or he may set the
# ``ClangTidy_EXECUTABLE`` variable directly.

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

# Search ClangTidy_ROOT_DIR exclusively if it is set.
if(ClangTidy_ROOT_DIR)
	set(_clang_tidy_search PATHS ${ClangTidy_ROOT_DIR} NO_DEFAULT_PATH)
else()
	set(_clang_tidy_search)
endif()

find_program(ClangTidy_EXECUTABLE
  NAMES
    clang-tidy-${ClangTidy_FIND_VERSION_MAJOR}.${ClangTidy_FIND_VERSION_MINOR}
    clang-tidy-${ClangTidy_FIND_VERSION_MAJOR}
    clang-tidy
  ${_clang_tidy_search}
  PATH_SUFFIXES bin
  DOC "The filepath of the clang-tidy executable"
)

if(ClangTidy_EXECUTABLE)
	execute_process(
	  COMMAND "${ClangTidy_EXECUTABLE}" -version
	  OUTPUT_VARIABLE _clang_tidy_version_string
	)
	string(REGEX MATCH "LLVM version ([0-9]+)[.]([0-9]+)[.]([0-9]+)" ignored "${_clang_tidy_version_string}")
	set(ClangTidy_VERSION_MAJOR "${CMAKE_MATCH_1}")
	set(ClangTidy_VERSION_MINOR "${CMAKE_MATCH_2}")
	set(ClangTidy_VERSION_PATCH "${CMAKE_MATCH_3}")
	set(ClangTidy_VERSION "${ClangTidy_VERSION_MAJOR}.${ClangTidy_VERSION_MINOR}.${ClangTidy_VERSION_PATCH}")
	set(ClangTidy_VERSION_COUNT 3)
endif()

# handle the QUIETLY and REQUIRED arguments and set ClangTidy_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ClangTidy
  REQUIRED_VARS
    ClangTidy_EXECUTABLE
  VERSION_VAR
    ClangTidy_VERSION
)

if(ClangTidy_FOUND)
	if(NOT TARGET Clang::Tidy)
		add_executable(Clang::Tidy IMPORTED)
		set_target_properties(Clang::Tidy PROPERTIES
		  IMPORTED_LOCATION "${ClangTidy_EXECUTABLE}")
	endif()
endif()
