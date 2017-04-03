#
#    Copyright 2014-2017 Kai Pastor
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


message(STATUS "Configuring ${PROJECT_NAME} licensing documentation")

find_file(LICENSING_COPYRIGHT_DIR
  NAMES copyright
  PATH_SUFFIXES share/doc
)
set(LICENSING_COPYRIGHT_DIR "${LICENSING_COPYRIGHT_DIR}" CACHE DIR
  "Path to directory with 3rd-party component copyright files"
)

find_file(LICENSING_COMMON_DIR
  NAMES common-licenses
  PATHS "${LICENSING_COPYRIGHT_DIR}"
  NO_DEFAULT_PATH
)
set(LICENSING_COMMON_DIR "${LICENSING_COMMON_DIR}" CACHE DIR
  "Path to directory with common license files"
)

message(STATUS "LICENSING_COPYRIGHT_DIR: ${LICENSING_COPYRIGHT_DIR}")
message(STATUS "LICENSING_COMMON_DIR:    ${LICENSING_COMMON_DIR}")


set(copyright_files_basenames
  curl-7*
  expat-2*
  gdal-2*
  libjpeg-turbo-1*
  libpng1.6-1*
  libpolyclipping-6*
  pcre3-8*
  proj-4*
  qtbase-5*
  qtimageformats-5*
  qtlocation-5*
  qtsensors-5*
  qtserialport-5*
  qttools-5*
  qttranslations-5*
  sqlite3-3*
  tiff-4*
  xz-utils-5*
  zlib-1*
)

set(common_licenses_basenames
  Apache-2.0
  Artistic
  BSD
  GFDL-1.3
  GPL-1
  GPL-2
  GPL-3
  LGPL-2
  LGPL-2.1
  LGPL-3
)

function(collect_files var type dir target)
	set(basenames ${ARGN})
	set(${var} )
	foreach(basename ${basenames})
		if(basename STREQUAL "GPL-3")
			set(name "GPL-3.txt")
			set(path "${PROJECT_SOURCE_DIR}/COPYING")
		else()
			file(GLOB path "${dir}/${basename}.txt")
			if(NOT path)
				message(WARNING "No ${type} file for ${basename}" )
				continue()
			elseif(path MATCHES ";")
				message(SEND_ERROR "Multiple ${type} files for ${basename}")
				continue()
			endif()
			get_filename_component(name "${path}" NAME)
			string(REPLACE ".txt" "" basename "${name}")
		endif()
		list(APPEND ${var} "  <li><a href=\"${target}/${name}\">${basename}</a></li>")
		install(FILES "${path}"
		  DESTINATION "${MAPPER_ABOUT_DESTINATION}/${target}"
		  RENAME "${basename}.txt"
		)
	endforeach()
	string(REPLACE ";" "\n" ${var} "${${var}}")
	set(${var} "${${var}}" PARENT_SCOPE)
endfunction()


set(third_party_copyright )
set(common_licenses )
if(LICENSING_COPYRIGHT_DIR AND LICENSING_COMMON_DIR)
	collect_files(third_party_copyright
	  "copyright"
	  "${LICENSING_COPYRIGHT_DIR}"
	  "3rd-party"
	  ${copyright_files_basenames}
	)
	
	collect_files(common_licenses
	  "license"
	  "${LICENSING_COMMON_DIR}"
	  "common-licenses"
	  ${common_licenses_basenames}
	)
	
elseif(LICENSING_COPYRIGHT_DIR OR LICENSING_COMMON_DIR)
	message(SEND_ERROR "Both LICENSING_COPYRIGHT_DIR and LICENSING_COMMON_DIR must be set")
	
else()
	message(WARNING "LICENSING_COPYRIGHT_DIR and LICENSING_COMMON_DIR are not set.")
	message(WARNING "Licensing documentation will be incomplete.")
	install(FILES "${PROJECT_SOURCE_DIR}/COPYING"
	  DESTINATION "${MAPPER_ABOUT_DESTINATION}/common-licenses"
	  RENAME "GPL-3.txt" # used by About dialog
	)
	
endif()
	

configure_file(licensing.html licensing.tmp.html)
file(GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/licensing.html
  INPUT ${CMAKE_CURRENT_BINARY_DIR}/licensing.tmp.html
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/licensing.html"
  DESTINATION "${MAPPER_ABOUT_DESTINATION}"
)

add_custom_target(Mapper-licensing ALL
  DEPENDS   licensing.html
)
