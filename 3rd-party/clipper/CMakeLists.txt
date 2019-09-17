#
#    Copyright 2013-2015 Kai Pastor
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


project(Clipper)

cmake_minimum_required(VERSION 2.8.3)

# Configuration options

set(CLIPPER_VERSION_DEFAULT 6.4.2)
if(CLIPPER_VERSION MATCHES "^5")
	unset(CLIPPER_VERSION CACHE) # source incompatible
endif()
set(CLIPPER_VERSION ${CLIPPER_VERSION_DEFAULT} CACHE STRING
  "Version number of the Clipper library")
mark_as_advanced(CLIPPER_VERSION)



message(STATUS "Configuring Clipper library ${CLIPPER_VERSION}")

if (NOT ${CLIPPER_VERSION} STREQUAL ${CLIPPER_VERSION_DEFAULT})
	message(WARNING 
	  "The Clipper library version is different from the current recommended version "
	  "(${CLIPPER_VERSION} vs. ${CLIPPER_VERSION_DEFAULT}).")
endif()

# Optionally use externally provided clipper source dir (e.g. Debian packaging)
set(CLIPPER_SOURCE_DIR_DEFAULT)
set(CLIPPER_SOURCE_DIR "${CLIPPER_SOURCE_DIR_DEFAULT}" CACHE STRING
  "The Clipper library source directory to be used instead of a download")
mark_as_advanced(CLIPPER_SOURCE_DIR)

if(CLIPPER_SOURCE_DIR)
	# Expand variables like @PROJECT_SOURCE_DIR@
	string(CONFIGURE "${CLIPPER_SOURCE_DIR}" EXPANDED_SOURCE_DIR @ONLY)
	set(CLIPPER_SOURCE
	  SOURCE_DIR "${EXPANDED_SOURCE_DIR}"
	)
else()
	set(CLIPPER_MD5SUMS
	  # Schema: VERSION:MD5
	  6.1.3a:4dcd043ce48de59714f07bd3ec7ac62b
	  6.4.2:100b4ec56c5308bac2d10f3966e35e11
	)
	foreach(line ${CLIPPER_MD5SUMS})
		if(${line} MATCHES "^${CLIPPER_VERSION}:")
			string(REPLACE "${CLIPPER_VERSION}:" "" CLIPPER_MD5 ${line})
			break()
		endif()
	endforeach()
	if(NOT CLIPPER_MD5)
		message(FATAL_ERROR
		  "Unknown MD5 sum for Clipper library ${CLIPPER_VERSION}. "
		  "Edit ${PROJECT_SOURCE_DIR}/CMakeLists.txt, "
		  "or specify the correct CLIPPER_MD5 value at the command line.")
	endif()
	set(CLIPPER_SOURCE
	  DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/download
	  URL "http://sourceforge.net/projects/polyclipping/files/clipper_ver${CLIPPER_VERSION}.zip/download"
	  URL_MD5 ${CLIPPER_MD5}
	)
endif()

set(CLIPPER_LICENSE_FILE "${PROJECT_SOURCE_DIR}/License.txt")
if(EXISTS "${CLIPPER_LICENSE_FILE}.${CLIPPER_VERSION}")
	set(CLIPPER_LICENSE_FILE "${CLIPPER_LICENSE_FILE}.${CLIPPER_VERSION}")
endif()
file(GLOB CLIPPER_LICENSE_FILES License.txt*)
add_custom_target(clipper-licenses
  COMMENT   "This target makes Qt Creator show all sources in the project tree."
  SOURCES   ${CLIPPER_LICENSE_FILES}
)

# External project definition

include(ExternalProject)
ExternalProject_Add(
  Clipper
  ${CLIPPER_SOURCE}
  CONFIGURE_COMMAND
    # Check that the license hasn't changed.
    ${CMAKE_COMMAND} -E compare_files <SOURCE_DIR>/License.txt "${CLIPPER_LICENSE_FILE}"
  COMMAND
    # Force source file timestamp update.
    ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/cpp/clipper.cpp "${CMAKE_CURRENT_BINARY_DIR}/clipper.cpp"
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(Clipper SOURCE_DIR)


# The actual library build

set(CMAKE_CXX_CLANG_TIDY "")
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "")

if(CMAKE_COMPILER_IS_GNUCXX)
	string(REPLACE "-Wpedantic" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

set(CLIPPER_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/clipper.cpp")
set_source_files_properties(${CLIPPER_SOURCES} PROPERTIES GENERATED TRUE)

add_library(polyclipping STATIC ${CLIPPER_SOURCES})

target_include_directories(polyclipping SYSTEM PUBLIC ${SOURCE_DIR}/cpp)

add_dependencies(polyclipping Clipper)
