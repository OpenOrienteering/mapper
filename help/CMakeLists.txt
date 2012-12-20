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

if(Mapper_USE_QT5)
	find_package(Qt5Core QUIET REQUIRED)
	get_target_property(QT5_QMAKE_EXECUTABLE ${Qt5Core_QMAKE_EXECUTABLE} IMPORTED_LOCATION)
	get_filename_component(QT5_QMAKE_DIR "${QT5_QMAKE_EXECUTABLE}" PATH)
	find_program(QT_QCOLLECTIONGENERATOR_EXECUTABLE
	  NAMES qcollectiongenerator
	  PATHS "${QT5_QMAKE_DIR}"
	  NO_DEFAULT_PATH
	  NO_CMAKE_FIND_ROOT_PATH
	)
	mark_as_advanced(QT_QCOLLECTIONGENERATOR_EXECUTABLE)
else()
	find_package(Qt4 QUIET REQUIRED)
endif()

if(QT_QCOLLECTIONGENERATOR_EXECUTABLE MATCHES NOTFOUND)
	message(FATAL_ERROR "qcollectiongenerator executable not found.")
endif()

# The manual must be compiled from the top-level directory,
# since qcollectiongenerator is not very flexible in this regard
 
set(Mapper_HELP_COLLECTION oomaphelpcollection.qhc)
set(Mapper_COMPRESSED_HELP oomaphelp.qch)
configure_file(oomaphelpcollection.qhcp.in oomaphelpcollection.qhcp)
file(GLOB Mapper_HELP_EN
  ${PROJECT_SOURCE_DIR}/images
  ${PROJECT_SOURCE_DIR}/help/html_en 
  ${PROJECT_SOURCE_DIR}/help/html_en/images
)
add_custom_command( 
	OUTPUT    ${Mapper_HELP_COLLECTION} ${Mapper_COMPRESSED_HELP}
 	COMMAND   ${QT_QCOLLECTIONGENERATOR_EXECUTABLE}
	ARGS      oomaphelpcollection.qhcp
	          -o ${Mapper_HELP_COLLECTION}
	MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/oomaphelp.qhp 
	DEPENDS   ${CMAKE_CURRENT_BINARY_DIR}/oomaphelpcollection.qhcp 
	          ${Mapper_HELP_EN}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT   "Compile help collection file"
)
add_custom_target(Mapper_manual ALL DEPENDS ${Mapper_HELP_COLLECTION})

# For super-builds (e.g. cross-compilation), wait for the Qt build.
add_dependencies(Mapper_manual Mapper_prerequisites)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${Mapper_HELP_COLLECTION}" 
        "${CMAKE_CURRENT_BINARY_DIR}/${Mapper_COMPRESSED_HELP}" 
  DESTINATION "${MAPPER_DATA_DESTINATION}/help")

