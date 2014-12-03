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


project(Doxygen NONE)

cmake_minimum_required(VERSION 2.8.3)

# Configuration options

set(DOXYGEN_VERSION_DEFAULT 1.8.8)
set(DOXYGEN_VERSION ${DOXYGEN_VERSION_DEFAULT} CACHE STRING
  "Version number of the doxygen library")
mark_as_advanced(DOXYGEN_VERSION)

message(STATUS "Configuring doxygen ${DOXYGEN_VERSION}")

if (NOT ${DOXYGEN_VERSION} STREQUAL ${DOXYGEN_VERSION_DEFAULT})
	message(WARNING 
	  "The doxygen library version is different from the current recommended version "
	  "(${DOXYGEN_VERSION} vs. ${DOXYGEN_VERSION_DEFAULT}).")
endif()

set(DOXYGEN_MD5SUMS
  # Schema: VERSION:MD5
  1.8.8:453892def7b378df387585a9358c23d4
)
foreach(line ${DOXYGEN_MD5SUMS})
	if(${line} MATCHES "^${DOXYGEN_VERSION}:")
		string(REPLACE "${DOXYGEN_VERSION}:" "" DOXYGEN_MD5 ${line})
		break()
	endif()
endforeach()
if(NOT DOXYGEN_MD5)
	message(FATAL_ERROR
	  "Unknown MD5 sum for doxygen library ${DOXYGEN_VERSION}. "
	  "Edit ${PROJECT_SOURCE_DIR}/CMakeLists.txt, "
	  "or specify the correct DOXYGEN_MD5 value at the command line.")
endif()
set(DOXYGEN_SOURCE
  DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/download
  URL "http://sourceforge.net/projects/doxygen/files/rel-${DOXYGEN_VERSION}/doxygen-${DOXYGEN_VERSION}.src.tar.gz/download"
  URL_MD5 ${DOXYGEN_MD5}
)

set(DOXYGEN_LICENSE_FILE "${PROJECT_SOURCE_DIR}/LICENSE")
if(EXISTS "${DOXYGEN_LICENSE_FILE}.${DOXYGEN_VERSION}")
	set(DOXYGEN_LICENSE_FILE "${DOXYGEN_LICENSE_FILE}.${DOXYGEN_VERSION}")
endif()
file(GLOB DOXYGEN_LICENSE_FILES LICENSE*)
add_custom_target(doxygen-licenses
  COMMENT   "This target makes Qt Creator show all sources in the project tree."
  SOURCES   ${DOXYGEN_LICENSE_FILES}
)

# External project definition

include(ExternalProject)
ExternalProject_Add(
  doxygen-project
  ${DOXYGEN_SOURCE}
  # Check that the license hasn't changed.
  UPDATE_COMMAND ${CMAKE_COMMAND} -E compare_files <SOURCE_DIR>/LICENSE "${DOXYGEN_LICENSE_FILE}"
  CONFIGURE_COMMAND "<SOURCE_DIR>/configure" "--release" "--prefix" "<INSTALL_DIR>"
  BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(doxygen-project INSTALL_DIR)


# Exported configuration

add_executable(doxygen IMPORTED GLOBAL)
set_target_properties(doxygen PROPERTIES
  IMPORTED_LOCATION "${INSTALL_DIR}/bin/doxygen"
)
add_dependencies(doxygen doxygen-project)
set(DOXYGEN_EXECUTABLE doxygen CACHE PATH
  "The path where to find doxygen"
  FORCE
)
mark_as_advanced(DOXYGEN_EXECUTABLE)
