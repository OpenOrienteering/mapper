#
#    Copyright 2012 Thomas Schöps, Kai Pastor
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

message(STATUS "Configuring ${PROJECT_NAME} help")

find_package(Qt5Help REQUIRED)
qt5_find_host_program(QT_QCOLLECTIONGENERATOR_EXECUTABLE NAMES qcollectiongenerator)
find_program(QT_QTASSISTANT_EXECUTABLE NAMES assistant${CMAKE_EXECUTABLE_SUFFIX} Assistant${CMAKE_EXECUTABLE_SUFFIX}
  DOC "The path of the Qt Assistant executable"
  HINTS ${QT5_DIR}
  PATH_SUFFIXES bin
  NO_DEFAULT_PATH)
mark_as_advanced(QT_QCOLLECTIONGENERATOR_EXECUTABLE)
mark_as_advanced(QT_QTASSISTANT_EXECUTABLE)

if(QT_QCOLLECTIONGENERATOR_EXECUTABLE MATCHES NOTFOUND)
	message(FATAL_ERROR "qcollectiongenerator executable not found.")
endif()

set(Mapper_HELP_NAMESPACE "openorienteering.mapper-${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}.help"
  CACHE INTERNAL "The namespace where the current version's help will be located.")

set(Mapper_HELP_COLLECTION "Mapper ${Mapper_VERSION_DISPLAY}.qhc")
set(Mapper_COMPRESSED_HELP "Mapper ${Mapper_VERSION_DISPLAY}.qch")
configure_file(Mapper.qhcp.in Mapper.qhcp)
configure_file(Mapper.qhp.in Mapper.qhp)
file(GLOB Mapper_HELP_EN
  ${PROJECT_SOURCE_DIR}/images
  ${PROJECT_SOURCE_DIR}/help/html_en 
  ${PROJECT_SOURCE_DIR}/help/html_en/images
)
add_custom_command( 
	OUTPUT    ${Mapper_HELP_COLLECTION} ${Mapper_COMPRESSED_HELP}
	COMMAND   ${CMAKE_COMMAND}
	ARGS      -E copy_directory ${PROJECT_SOURCE_DIR}/help/html_en
	            ${CMAKE_CURRENT_BINARY_DIR}/help/html_en
	COMMAND   ${CMAKE_COMMAND}
	ARGS      -E copy_directory ${PROJECT_SOURCE_DIR}/images
	            ${CMAKE_CURRENT_BINARY_DIR}/images
	COMMAND   ${QT_QCOLLECTIONGENERATOR_EXECUTABLE}
	ARGS      Mapper.qhcp
	          -o ${Mapper_HELP_COLLECTION}
	MAIN_DEPENDENCY ${CMAKE_CURRENT_BINARY_DIR}/Mapper.qhp 
	DEPENDS   ${CMAKE_CURRENT_BINARY_DIR}/Mapper.qhcp 
	          ${Mapper_HELP_EN}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT   "Compiling help"
)
add_custom_target(Mapper_manual ALL DEPENDS ${Mapper_HELP_COLLECTION})

# For super-builds (e.g. cross-compilation), wait for the Qt build.
add_dependencies(Mapper_manual Mapper_prerequisites)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${Mapper_HELP_COLLECTION}" 
        "${CMAKE_CURRENT_BINARY_DIR}/${Mapper_COMPRESSED_HELP}" 
  DESTINATION "${MAPPER_DATA_DESTINATION}/help")

message(STATUS "Configuring ${PROJECT_NAME} help - done")
