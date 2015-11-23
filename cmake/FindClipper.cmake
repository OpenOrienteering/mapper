#
#    Copyright 2014, 2015 Kai Pastor
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

# If the system's pkgconfig finds libpolyclipping (aka Clipper),  creates a
# target polyclipping (library, imported), and sets CLIPPER_FOUND to true.

find_package(PkgConfig)
pkg_check_modules(POLYCLIPPING QUIET polyclipping)

find_library(POLYCLIPPING_LIBRARY_LOCATION
  NAMES polyclipping
  PATHS ${POLYCLIPPING_LIBDIR}
  NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Clipper
  REQUIRED_VARS POLYCLIPPING_LIBRARY_LOCATION POLYCLIPPING_LIBDIR POLYCLIPPING_INCLUDE_DIRS
  VERSION_VAR   POLYCLIPPING_VERSION
)

add_library(polyclipping IMPORTED UNKNOWN)
set_target_properties(polyclipping PROPERTIES
  IMPORTED_LOCATION             "${POLYCLIPPING_LIBRARY_LOCATION}"
  INTERFACE_COMPILE_DEFINITIONS "${POLYCLIPPING_CFLAGS_OTHER}"
  # We can't rely on POLYCLIPPING_INCLUDE_DIRS:
  # Clipper-6.1.3a shipped with broken CFlags.
  INTERFACE_INCLUDE_DIRECTORIES "${POLYCLIPPING_INCLUDEDIR}"
)
