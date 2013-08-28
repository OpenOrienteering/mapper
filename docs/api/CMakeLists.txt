#
#    Copyright 2012, 2013 Kai Pastor
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

message(STATUS "Configuring ${PROJECT_NAME} API documentation")

find_program(DOXYGEN_EXECUTABLE NAMES doxygen
	  DOC "The path of the doxygen executable")
mark_as_advanced(DOXYGEN_EXECUTABLE)

if(DOXYGEN_EXECUTABLE MATCHES NOTFOUND)
	message(STATUS "doxygen executable not found.")
else()
	configure_file(${PROJECT_NAME}.doxygen.in ${PROJECT_NAME}.doxygen @ONLY)
	add_custom_target(api-docs
	  COMMAND   ${CMAKE_COMMAND} -E remove_directory html
	  COMMAND   ${DOXYGEN_EXECUTABLE} ${PROJECT_NAME}.doxygen
	  COMMENT   "Generating API documentation"
	)
	add_custom_target(api-docs-package
	  COMMAND ${CMAKE_COMMAND} -E tar czf "${PROJECT_BINARY_DIR}/api-docs.tgz"
	                                       api/html/
	  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
	)
	add_dependencies(api-docs-package api-docs)
endif()

set(API_RSYNC_USER unknown
  CACHE STRING "The rsync user for uploading API docs (SF account)")
if(API_RSYNC_TARGET MATCHES "unknown")
	set(API_RSYNC_USER_FORCE FORCE)
endif()
set(API_RSYNC_TARGET
  "${API_RSYNC_USER},oorienteering@web.sourceforge.net:htdocs/api/"
  CACHE STRING "The rsync target for uploading API docs"
  ${API_RSYNC_USER_FORCE})
mark_as_advanced(API_RSYNC_USER API_RSYNC_TARGET)

if(DOXYGEN_EXECUTABLE AND NOT "${API_RSYNC_TARGET}" MATCHES "unknown")
	add_custom_target(api-docs-rsync
	  COMMAND rsync -avzP -e ssh html/ "${API_RSYNC_TARGET}"
	  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)
	add_dependencies(api-docs-rsync api-docs)
endif()

message(STATUS "Configuring ${PROJECT_NAME} API documentation - done")
