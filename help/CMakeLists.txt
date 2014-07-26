#
#    Copyright 2012, 2013, 2014 Thomas Schöps, Kai Pastor
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

# Not exported from Qt5
if (NOT TARGET Qt5::qcollectiongenerator AND TARGET Qt5::moc)
	add_executable(Qt5::qcollectiongenerator IMPORTED)
	get_target_property(imported_location Qt5::moc IMPORTED_LOCATION)
	get_filename_component(imported_location ${imported_location} PATH)
	set(imported_location ${imported_location}/qcollectiongenerator)
	if(NOT EXISTS "${imported_location}")
		message(FATAL_ERROR "qcollectiongenerator executable does not exist: "
		                    "${imported_location}")
	endif()
	set_target_properties(Qt5::qcollectiongenerator PROPERTIES
		IMPORTED_LOCATION ${imported_location}
	)
endif()
set(Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE Qt5::qcollectiongenerator)

find_program(Qt5Help_ASSISTANT_EXECUTABLE NAMES assistant${CMAKE_EXECUTABLE_SUFFIX} Assistant${CMAKE_EXECUTABLE_SUFFIX}
  DOC "The path of the Qt Assistant executable"
  HINTS ${QT5_DIR}
  PATH_SUFFIXES bin
  NO_DEFAULT_PATH)
mark_as_advanced(Qt5Help_ASSISTANT_EXECUTABLE)

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
	COMMAND   ${Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE}
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
