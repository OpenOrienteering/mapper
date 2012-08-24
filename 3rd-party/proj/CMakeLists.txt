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


project(Proj)

cmake_minimum_required(VERSION 2.8)

# Configuration options

if(NOT PROJ_VERSION)
	set(PROJ_VERSION 4.8.0 CACHE STRING
	  "Version number of the PROJ.4 library, minumum recommended value: 4.8.0"
	  FORCE)
endif(NOT PROJ_VERSION)
mark_as_advanced(PROJ_VERSION)

if(NOT PROJ_MD5)
	set(PROJ_MD5 d815838c92a29179298c126effbb1537 CACHE STRING
	  "MD5 checksum of the PROJ.4 library TGZ archive"
	  FORCE)
endif(NOT PROJ_MD5)
mark_as_advanced(PROJ_MD5)

if("${CMAKE_GENERATOR} ${CMAKE_EXTRA_GENERATOR}" MATCHES "Unix Makefiles" OR
   "${CMAKE_GENERATOR}" STREQUAL "MSYS Makefiles")
	# use magic MAKE macro
	set(PROJ_MAKE_COMMAND "\$(MAKE)" CACHE STRING
	  "The 'make' command for building the Proj library")
else()
	# use wrapper
	set(PROJ_MAKE_COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/proj-make CACHE STRING
	  "The 'make' command for building the Proj library")
endif()
mark_as_advanced(PROJ_MAKE_COMMAND)

find_program(SH_PROGRAM sh 
  PATHS 
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS-1.0_is1;Inno Setup: App Path]/bin"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MinGW;InstallLocation]/bin"
  C:
  C:/MinGW
  PATH_SUFFIXES msys/1.0/bin)
mark_as_advanced(SH_PROGRAM)

# Build definition

if(CMAKE_VERSION VERSION_GREATER 2.8.1)
	set(_ep_proj_options URL_MD5 ${PROJ_MD5} TIMEOUT 30)
else()
	set(_ep_proj_options)
endif()

include(ExternalProject)
ExternalProject_Add(
  Proj
  DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}
  URL "http://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz"
  ${_ep_proj_options}
  CONFIGURE_COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/proj-config
  BUILD_COMMAND ${PROJ_MAKE_COMMAND}
  INSTALL_COMMAND ${PROJ_MAKE_COMMAND} install
)
ExternalProject_Get_Property(Proj SOURCE_DIR)
ExternalProject_Get_Property(Proj BINARY_DIR)
ExternalProject_Get_Property(Proj INSTALL_DIR)

if(CMAKE_CROSSCOMPILING AND GNU_SYSTEM_NAME)
	set(_proj_cfg_cross "--build=$(sh ${SOURCE_DIR}/config.guess) --host=${GNU_SYSTEM_NAME} --target=${GNU_SYSTEM_NAME}")
else(CMAKE_CROSSCOMPILING AND GNU_SYSTEM_NAME)
	unset(_proj_cfg_cross)
endif(CMAKE_CROSSCOMPILING AND GNU_SYSTEM_NAME)

configure_file(proj-config.in ${CMAKE_CURRENT_BINARY_DIR}/proj-config)
configure_file(proj-make.in ${CMAKE_CURRENT_BINARY_DIR}/proj-make)

# Exported configuration

# The following will not succeed during the initial configuration
# but on repeated configuration after an successful build.

find_path(PROJ_INCLUDE_DIR NAMES proj_api.h PATHS
  "${INSTALL_DIR}/include"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
mark_as_advanced(PROJ_INCLUDE_DIR)

find_library(PROJ_LIBRARY NAMES proj proj_i PATHS
  "${INSTALL_DIR}/lib"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
mark_as_advanced(PROJ_LIBRARY)

if(PROJ_INCLUDE_DIR AND PROJ_LIBRARY)
	set(PROJ_BINARY_DIR "${INSTALL_DIR}/bin" PARENT_SCOPE)
	mark_as_advanced(PROJ_BINARY_DIR)
	set(PROJ_FOUND TRUE PARENT_SCOPE)
endif()

