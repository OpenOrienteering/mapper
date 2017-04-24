#
#    Copyright 2017 Kai Pastor
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


find_file(LICENSING_COPYRIGHT_DIR
  NAMES copyright
  PATH_SUFFIXES share/doc
  DOC "Path to directory with 3rd-party component copyright files."
)

find_file(LICENSING_COMMON_DIR
  NAMES common-licenses
  PATHS "${LICENSING_COPYRIGHT_DIR}"
  NO_DEFAULT_PATH
  DOC "Path to directory with common license files."
)

if(NOT LICENSING_COPYRIGHT_DIR OR NOT LICENSING_COMMON_DIR)
	message(STATUS "LICENSING_COPYRIGHT_DIR: ${LICENSING_COPYRIGHT_DIR}")
	message(STATUS "LICENSING_COMMON_DIR:    ${LICENSING_COMMON_DIR}")
	message(FATAL_ERROR "Both LICENSING_COPYRIGHT_DIR and LICENSING_COMMON_DIR must be set")
endif()	


# Based on OpenOrienteering superbuild as of 2017-04
list(APPEND third_party_components
  libcurl
  libexpat
  libjpeg-turbo
  liblzma
  libpcre3
  libpng
  libsqlite
  libtiff
  zlib
)
if(ANDROID OR MINGW)
	list(APPEND third_party_components gnustl)
endif()

list(APPEND common_license_names
  Apache-2.0
  Artistic
  BSD
  GFDL-1.3
  GPL-1
  GPL-2
  LGPL-2
  LGPL-2.1
  LGPL-3
)


# A 3rd-party Mapper component may consist of multiple DEB packages sharing the
# same copyright file. We reference it just once.
set(package_names
  libcurl:curl
  libexpat:expat
  libpcre3:pcre3
  libsqlite:sqlite3
  libtiff:tiff
)

set(copyright_pattern "${LICENSING_COPYRIGHT_DIR}/@package@*.txt")
set(common_license_pattern "${LICENSING_COMMON_DIR}/@basename@.txt")
