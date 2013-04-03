#
#    Copyright 2013 Kai Pastor
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

set(CLIPPER_VERSION_DEFAULT 5.1.4)
set(CLIPPER_VERSION ${CLIPPER_VERSION_DEFAULT} CACHE STRING
  "Version number of the Clipper library, minumum recommended value: 4.10.0")
mark_as_advanced(CLIPPER_VERSION)

if(CLIPPER_VERSION STREQUAL 4.10.0)
	set(CLIPPER_MD5 64dd3cc88b8238a02aae750d4c0a0be3)
elseif(CLIPPER_VERSION STREQUAL 5.0.2)
	set(CLIPPER_MD5 879749c46f5c2608d1f42a9090dc47d7)
elseif(CLIPPER_VERSION STREQUAL 5.0.3)
	set(CLIPPER_MD5 8509ee63d716e46e88a21ab31b315411)
elseif(CLIPPER_VERSION STREQUAL 5.1.4)
	set(CLIPPER_MD5 8693d4ca96f50d93ae36ea5761e07119)	
endif()
mark_as_advanced(CLIPPER_MD5)

message(STATUS "Configuring Clipper library ${CLIPPER_VERSION}")

# External project definition

include(ExternalProject)
ExternalProject_Add(
  Clipper
  DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/download
  URL "http://sourceforge.net/projects/polyclipping/files/clipper_ver${CLIPPER_VERSION}.zip/download"
  URL_MD5 ${CLIPPER_MD5}
  # Check that the license hasn't changed.
  UPDATE_COMMAND ${CMAKE_COMMAND} -E compare_files <SOURCE_DIR>/License.txt "${PROJECT_SOURCE_DIR}/License.txt"
  # Force source file timestamp update.
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/cpp/clipper.cpp "${CMAKE_CURRENT_BINARY_DIR}/clipper.cpp"
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(Clipper SOURCE_DIR)


# Exported configuration

set(CLIPPER_INCLUDE_DIR ${SOURCE_DIR}/cpp CACHE STRING
  "The directory where to find clipper.hpp")
mark_as_advanced(CLIPPER_INCLUDE_DIR)

set(CLIPPER_LIBRARY polyclipping CACHE STRING
  "The Clipper C++ library.")
mark_as_advanced(CLIPPER_LIBRARY)


# The actual library build

set(CLIPPER_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/clipper.cpp")
set_source_files_properties(${CLIPPER_SOURCES} PROPERTIES GENERATED TRUE)
include_directories(${CLIPPER_INCLUDE_DIR})
add_library(${CLIPPER_LIBRARY} STATIC ${CLIPPER_SOURCES})
add_dependencies(${CLIPPER_LIBRARY} Clipper)


message(STATUS "Configuring Clipper library - done")

