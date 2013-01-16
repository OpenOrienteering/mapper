#
#    Copyright 2012 Kai Pastor
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
	add_custom_target(doc
	  COMMAND   ${CMAKE_COMMAND} -E remove_directory html
	  COMMAND   ${DOXYGEN_EXECUTABLE} ${PROJECT_NAME}.doxygen
	  COMMENT   "Generating API documentation"
	)
	add_custom_target(doc-package
	  COMMAND ${CMAKE_COMMAND} -E tar czf "${PROJECT_BINARY_DIR}/api-docs.tgz"
	                                       api/html/
	  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
	)
	add_dependencies(doc-package doc)
endif()

message(STATUS "Configuring ${PROJECT_NAME} API documentation - done")
