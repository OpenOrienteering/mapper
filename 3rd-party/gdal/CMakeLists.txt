#
#    Copyright 2016 Kai Pastor
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


project(GDAL)

cmake_minimum_required(VERSION 2.8.3)

include(ExternalProject)

# Configuration options

set(GDAL_VERSION_DEFAULT 2.0.1+dfsg)
set(GDAL_VERSION ${GDAL_VERSION_DEFAULT} CACHE STRING
  "Version number of the GDAL library, recommended value: ${GDAL_VERSION_DEFAULT}")
mark_as_advanced(GDAL_VERSION)

message(STATUS "Configuring GDAL library ${GDAL_VERSION}")

string(REGEX MATCH "^[0-9]\\.[0-9]+\\.[0-9]+" GDAL_MAJOR_MINOR_REVISION ${GDAL_VERSION})
if (NOT "${GDAL_VERSION}" MATCHES "^${GDAL_MAJOR_MINOR_REVISION}")
	message(WARNING
	  "The GDAL library version is different from the current recommended version "
	  "(${GDAL_VERSION} vs. ${GDAL_VERSION_DEFAULT}).")
endif()

set(GDAL_MD5SUMS
  2.0.1+dfsg:18e207a12f920e2a40405891eb6168ec:https://github.com/OpenOrienteering/sources/releases/download/3rd-party/gdal_2.0.1.dfsg.orig.tar.gz
)
string(REPLACE "+" "[+]" GDAL_VERSION_REGEX "${GDAL_VERSION}")
foreach(line ${GDAL_MD5SUMS})
	if(${line} MATCHES "^${GDAL_VERSION_REGEX}:")
		string(REPLACE "${GDAL_VERSION}:" "" GDAL_MD5 ${line})
		break()
	endif()
endforeach()
if(NOT GDAL_MD5)
	message(FATAL_ERROR
	  "Unknown MD5 sum for GDAL library ${GDAL_VERSION}. "
	  "Edit ${GDALECT_SOURCE_DIR}/CMakeLists.txt, "
	  "or specify the correct GDAL_MD5 value at the command line.")
endif()
if(GDAL_MD5 MATCHES ":")
	message(WARNING "Not using an official release of GDAL.")
	string(REGEX REPLACE "^[0-9a-fA-F]*:" "" GDAL_URL "${GDAL_MD5}")
	string(REGEX REPLACE ":.*"      "" GDAL_MD5 "${GDAL_MD5}")
elseif(GDAL_VERSION MATCHES "-openorienteering-")
	set(GDAL_URL "https://github.com/OpenOrienteering/sources/releases/download/3rd-party/gdal-${GDAL_VERSION}.tar.gz")
else()
	set(GDAL_URL "http://download.osgeo.org/gdal/${GDAL_VERSION}/gdal-${GDAL_VERSION}.tar.gz")
endif()

set(GDAL_LICENSE_FILE "${PROJECT_SOURCE_DIR}/LICENSE.TXT")
if(EXISTS "${GDAL_LICENSE_FILE}.${GDAL_VERSION}")
	set(GDAL_LICENSE_FILE "${GDAL_LICENSE_FILE}.${GDAL_VERSION}")
endif()


# External project definition

set(GDAL_EXTRA_FLAGS "-Wno-unused-parameter -Wno-ignored-qualifiers -Wno-unused-private-field -Wno-deprecated-register")
string(REGEX REPLACE "-Wpedantic|-Wall" "" GDAL_CFLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE} ${GDAL_EXTRA_FLAGS}")
string(REGEX REPLACE "-Wpedantic|-Wall" "" GDAL_CXXFLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${GDAL_EXTRA_FLAGS}")
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
	execute_process(
	  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/patches/config.guess"
	  OUTPUT_VARIABLE build
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	  )
	set(GDAL_CONFIG_EXTRA "--host=${GNU_SYSTEM_NAME}" "--build=${build}" --without-expat)
	get_directory_property(_include_dirs INCLUDE_DIRECTORIES)
	if(_include_dirs)
		unset(_cpp_flags)
		foreach(_dir ${_include_dirs})
			set(_cpp_flags "${_cpp_flags} -I${_dir}")
		endforeach()
		list(APPEND GDAL_CONFIG_EXTRA "CPPFLAGS=${_cpp_flags}")
	endif()
	if(GNU_SYSTEM_NAME STREQUAL "arm-linux-androideabi")
		set(STANDALONE_TOOLCHAIN arm-linux-androideabi-4.9)
		set(CMAKE_C_COMPILER <BINARY_DIR>/toolchain/${STANDALONE_TOOLCHAIN}/bin/arm-linux-androideabi-gcc)
		set(CMAKE_CXX_COMPILER <BINARY_DIR>/toolchain/${STANDALONE_TOOLCHAIN}/bin/arm-linux-androideabi-g++)
	elseif(GNU_SYSTEM_NAME STREQUAL "i686-linux-android")
		set(STANDALONE_TOOLCHAIN arm-linux-androideabi-gcc-4.9)
		set(CMAKE_C_COMPILER <BINARY_DIR>/toolchain/${STANDALONE_TOOLCHAIN}/bin/i686-linux-android-gcc)
		set(CMAKE_CXX_COMPILER <BINARY_DIR>/toolchain/${STANDALONE_TOOLCHAIN}/bin/i686-linux-android-g++)
	elseif(ANDROID)
		message(WARNING "Unsupported Android architecture '${GNU_SYSTEM_NAME}'")
	endif()
elseif(APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
	# Didn't manage to pass the deployment target to all libtool link steps
	#string(REGEX REPLACE "(^| )(--?[^W])" "\\1-Wl,\\2" CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
	set(GDAL_CONFIG_EXTRA --without-libtool) 
else()
	unset(GDAL_CONFIG_EXTRA)
endif()

include(ExternalProject)
ExternalProject_Add(
  GDAL
  DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/download
  URL ${GDAL_URL}
  URL_MD5 ${GDAL_MD5}
  # Check that the license hasn't changed.
  UPDATE_COMMAND
    ${CMAKE_COMMAND} -E compare_files <SOURCE_DIR>/LICENSE.TXT "${GDAL_LICENSE_FILE}"
  PATCH_COMMAND
    cp ${PROJECT_SOURCE_DIR}/patches/config.sub ${PROJECT_SOURCE_DIR}/patches/config.guess <SOURCE_DIR>/
  COMMAND
    find ${PROJECT_SOURCE_DIR}/patches -name gdal-${GDAL_MAJOR_MINOR_REVISION}*.patch -exec patch -p0 -N -i {} +
  CONFIGURE_COMMAND
    "<SOURCE_DIR>/configure"
    "--prefix=<INSTALL_DIR>"
    --disable-static
    --enable-shared
    --with-ogr
    --with-hide-internal-symbols
    --with-rename-internal-libtiff-symbols
    --without-jpeg12 # possible violation of ODR in GDAL < 2.1
    --with-threads=no
    --without-pcraster
    --without-grib
    --without-perl
    --without-php
    --without-python
    --without-java
    ${GDAL_CONFIG_EXTRA}
    "CC=${CMAKE_C_COMPILER}"
    "CFLAGS=${GDAL_CFLAGS}"
    "CXX=${CMAKE_CXX_COMPILER}"
    "CXXFLAGS=${GDAL_CXXFLAGS}"
    "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS}"
  BUILD_COMMAND
    "\$(MAKE)"
  INSTALL_COMMAND
    "\$(MAKE)" -j1 install
  BUILD_IN_SOURCE 1
)

if(ANDROID AND STANDALONE_TOOLCHAIN)
	ExternalProject_Add_Step(GDAL toolchain
	  COMMENT "Creating standalone toolchain"
	  DEPENDEES update
	  DEPENDERS configure
	  COMMAND
	    bash $(ANDROID_NDK_ROOT)/build/tools/make-standalone-toolchain.sh
	      --install-dir=<BINARY_DIR>/toolchain/${STANDALONE_TOOLCHAIN}
	      --platform=$(ANDROID_NDK_PLATFORM)
	      --toolchain=${STANDALONE_TOOLCHAIN}
	)
endif()


# Exported configuration

# The following will not succeed during the initial configuration
# but on repeated configuration after an successful build.

ExternalProject_Get_Property(GDAL BINARY_DIR)
ExternalProject_Get_Property(GDAL INSTALL_DIR)

find_path(GDAL_INCLUDE_DIR NAMES gdal.h PATHS
  "${INSTALL_DIR}/include"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
mark_as_advanced(GDAL_INCLUDE_DIR)

find_library(GDAL_LIBRARY NAMES gdal PATHS
  "${INSTALL_DIR}/lib"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
mark_as_advanced(GDAL_LIBRARY)

if(GDAL_INCLUDE_DIR AND GDAL_LIBRARY)
	set(GDAL_BINARY_DIR "${INSTALL_DIR}/bin" PARENT_SCOPE)
	mark_as_advanced(GDAL_BINARY_DIR)
	set(GDAL_FOUND TRUE PARENT_SCOPE)
endif()

# Don't let Xcode re-root the install
set_target_properties(GDAL PROPERTIES XCODE_ATTRIBUTE_INSTALL_ROOT "")
