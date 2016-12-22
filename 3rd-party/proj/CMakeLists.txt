#
#    Copyright 2012-2015 Kai Pastor
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


project(Proj C)

cmake_minimum_required(VERSION 2.8.3)

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES " ")
	# Note: autotools don't allow whitespace and some other special characters.
	message(FATAL_ERROR "The build directory must not contain whitespace.")
endif()

# Configuration options

set(PROJ_VERSION_DEFAULT 4.9.2)
set(PROJ_VERSION ${PROJ_VERSION_DEFAULT} CACHE STRING
  "Version number of the Proj library, recommended value: ${PROJ_VERSION_DEFAULT}")
mark_as_advanced(PROJ_VERSION)

message(STATUS "Configuring Proj library ${PROJ_VERSION}")

if (NOT ${PROJ_VERSION} STREQUAL ${PROJ_VERSION_DEFAULT})
	message(WARNING
	  "The Proj library version is different from the current recommended version "
	  "(${PROJ_VERSION} vs. ${PROJ_VERSION_DEFAULT}).")
endif()

set(PROJ_MD5SUMS
  4.9.2:9843131676e31bbd903d60ae7dc76cf9
)
foreach(line ${PROJ_MD5SUMS})
	if(${line} MATCHES "^${PROJ_VERSION}:")
		string(REPLACE "${PROJ_VERSION}:" "" PROJ_MD5 ${line})
		break()
	endif()
endforeach()
if(NOT PROJ_MD5)
	message(FATAL_ERROR
	  "Unknown MD5 sum for Proj library ${PROJ_VERSION}. "
	  "Edit ${PROJECT_SOURCE_DIR}/CMakeLists.txt, "
	  "or specify the correct PROJ_MD5 value at the command line.")
endif()

set(PROJ_LICENSE_FILE "${PROJECT_SOURCE_DIR}/COPYING")
if(EXISTS "${PROJ_LICENSE_FILE}.${PROJ_VERSION}")
	set(PROJ_LICENSE_FILE "${PROJ_LICENSE_FILE}.${PROJ_VERSION}")
endif()

find_program(SH_PROGRAM sh
  PATHS
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS-1.0_is1;Inno Setup: App Path]/bin"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MinGW;InstallLocation]/bin"
  C:
  C:/MinGW
  PATH_SUFFIXES msys/1.0/bin)
mark_as_advanced(SH_PROGRAM)

if("${CMAKE_GENERATOR} ${CMAKE_EXTRA_GENERATOR}" MATCHES "Unix Makefiles" OR
   "${CMAKE_GENERATOR}" STREQUAL "MSYS Makefiles")
	# use magic MAKE macro
	set(PROJ_MAKE_COMMAND "\$(MAKE)")
else()
	# use wrapper
	set(PROJ_MAKE_COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/proj-make)
endif()

# External project definition

include(ExternalProject)
ExternalProject_Add(
  Proj
  DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/download
  URL "http://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz"
  URL_MD5 ${PROJ_MD5}
  PATCH_COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/proj-patch
  CONFIGURE_COMMAND
    # Check that the license hasn't changed.
    ${CMAKE_COMMAND} -E compare_files <SOURCE_DIR>/COPYING "${PROJ_LICENSE_FILE}"
  COMMAND
    "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/proj-config
  BUILD_COMMAND ${PROJ_MAKE_COMMAND}
  INSTALL_COMMAND ${PROJ_MAKE_COMMAND} -j1 install
               && "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/proj-postinstall
)
ExternalProject_Get_Property(Proj SOURCE_DIR)
ExternalProject_Get_Property(Proj BINARY_DIR)
ExternalProject_Get_Property(Proj INSTALL_DIR)

if(CMAKE_CROSSCOMPILING AND NOT GNU_SYSTEM_NAME AND MINGW)
	set(_env_lang $ENV{LC_ALL})
	set(ENV{LC_ALL} C)
	execute_process(
	  COMMAND ${CMAKE_C_COMPILER} -v
	  ERROR_VARIABLE GNU_SYSTEM_NAME
	)
	set(ENV{LC_ALL} ${_env_lang})
	string(REGEX REPLACE ".*Target: ?([^\n]*).*" \\1 GNU_SYSTEM_NAME ${GNU_SYSTEM_NAME})
endif()
if(CMAKE_CROSSCOMPILING AND GNU_SYSTEM_NAME)
	set(_proj_cfg_cross "--build=$(sh ${SOURCE_DIR}/config.guess) --host=${GNU_SYSTEM_NAME}")
	get_directory_property(_include_dirs INCLUDE_DIRECTORIES)
	if(_include_dirs)
		unset(_cpp_flags)
		foreach(_dir ${_include_dirs})
			set(_cpp_flags "${_cpp_flags} -I${_dir}")
		endforeach()
		set(_proj_cfg_cross "${_proj_cfg_cross} CPPFLAGS=\"${_cpp_flags}\"")
	endif()
else()
	unset(_proj_cfg_cross)
endif()

configure_file(proj-patch.in ${CMAKE_CURRENT_BINARY_DIR}/proj-patch)
configure_file(proj-config.in ${CMAKE_CURRENT_BINARY_DIR}/proj-config)
configure_file(proj-make.in ${CMAKE_CURRENT_BINARY_DIR}/proj-make)
configure_file(proj-postinstall.in ${CMAKE_CURRENT_BINARY_DIR}/proj-postinstall)

# Exported configuration

set(PROJ4_ROOT "${INSTALL_DIR}" PARENT_SCOPE)

# Remove any left-over cache entries
unset(PROJ4_ROOT CACHE)
unset(PROJ4_INCLUDE_DIR CACHE)
unset(PROJ4_LIBRARY CACHE)
unset(PROJ4_LIBRARY_DEBUG CACHE)
unset(PROJ4_LIBRARY_RELEASE CACHE)


# Don't let Xcode re-root the install
set_target_properties(Proj PROPERTIES XCODE_ATTRIBUTE_INSTALL_ROOT "")


message(STATUS "Configuring Proj library ${PROJ_VERSION} - done")

