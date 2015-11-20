#
#    Copyright 2014 Kai Pastor
#    
#    This file is part of OpenOrienteering.
# 
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.

#  Cf. http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
#
#  CLIPPER_FOUND        - System has libpolyclipping (aka Clipper)
#  CLIPPER_INCLUDE_DIRS - The Clipper include directories
#  CLIPPER_LIBRARIES    - The libraries needed to use Clipper
#  CLIPPER_DEFINITIONS  - Compiler switches required for using Clipper

find_package(PkgConfig)
pkg_check_modules(PC_CLIPPER QUIET polyclipping)

set(CLIPPER_DEFINITIONS ${PC_CLIPPER_CFLAGS_OTHER})

find_path(CLIPPER_INCLUDE_DIR
  clipper.hpp
  HINTS ${PC_CLIPPER_INCLUDEDIR} ${PC_CLIPPER_INCLUDE_DIRS}
)

find_library(CLIPPER_LIBRARY
  NAMES polyclipping
  HINTS ${PC_CLIPPER_LIBDIR} ${PC_CLIPPER_LIBRARY_DIRS}
)

set(CLIPPER_INCLUDE_DIRS ${CLIPPER_INCLUDE_DIR})
set(CLIPPER_LIBRARIES ${CLIPPER_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Clipper
  DEFAULT_MSG
  CLIPPER_LIBRARY
  CLIPPER_INCLUDE_DIR
)

mark_as_advanced(CLIPPER_INCLUDE_DIR CLIPPER_LIBRARY)
