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

if(PROJECT_NAME)
	message(FATAL_ERROR "bootstrap.cmake must be used as a script, not during project configuration")
endif()

cmake_minimum_required(VERSION 2.8.3)

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")

include("${CMAKE_CURRENT_LIST_DIR}/functions.cmake")

message(STATUS "Bootstrapping MinGW-builds")
if(NOT MSYS_URL)
# rev8 works well for me
#	set(MSYS_VERSION msys+7za+wget+svn+git+mercurial+cvs-rev8)
#	set(MSYS_MD5  10e79cf13642655d27b7e06a74b64f36)
# rev13 is the latest version
	set(MSYS_VERSION msys+7za+wget+svn+git+mercurial+cvs-rev13)
	set(MSYS_MD5  4c79f989eb6353a0d81bc39b6f7176ea)
	set(MSYS_URL  http://sourceforge.net/projects/mingwbuilds/files/external-binary-packages/${MSYS_VERSION}.7z)
endif()
if(NOT MSYS_DIR)
	set(MSYS_DIR ${CMAKE_CURRENT_BINARY_DIR}/msys)
endif()
if(NOT MSYS_SH_PROGRAM)
	set(MSYS_SH_PROGRAM ${MSYS_DIR}/bin/sh.exe)
endif()
if(NOT CMAKE_MAKE_PROGRAM OR NOT CMAKE_GENERATOR)
	set(CMAKE_MAKE_PROGRAM "${MSYS_DIR}/bin/make.exe")
	set(CMAKE_GENERATOR "MSYS Makefiles")
#	set(CMAKE_MAKE_PROGRAM "${MSYS_DIR}/temp/mingw-builds/toolchains/mingw32/bin/mingw32-make.exe")
#	set(CMAKE_GENERATOR "MinGW Makefiles")
endif()
require_definitions(MSYS_DIR MSYS_URL MSYS_SH_PROGRAM CMAKE_GENERATOR CMAKE_MAKE_PROGRAM)
if(NOT EXISTS ${MSYS_SH_PROGRAM})
	
	require_definitions(MSYS_URL MSYS_MD5)
	if ("${MSYS_URL}" MATCHES "\\.7[zZ]$")
		
		find_program(SEVENZA_PROGRAM 7za)
		if(SEVENZA_PROGRAM MATCHES NOTFOUND)
			find_program(SEVENZA_PROGRAM 7za PATHS "${MSYS_DIR}/bin" NO_DEFAULT_PATH)
			if (NOT SEVENZA_PROGRAM MATCHES NOTFOUND)
				set(ENV{PATH} "$ENV{PATH};${MSYS_DIR}/bin")
			else()
				message(STATUS "Setting up 7za")
				set(SEVENZA_DIR "${CMAKE_CURRENT_BINARY_DIR}/7z")
				if(NOT SEVENZA_URL)
					set(SEVENZA_VERSION_MAJOR 9)
					set(SEVENZA_VERSION_MINOR 20)
					set(SEVENZA_URL           http://sourceforge.net/projects/sevenzip/files/7-Zip/${SEVENZA_VERSION_MAJOR}.${SEVENZA_VERSION_MINOR}/7za${SEVENZA_VERSION_MAJOR}${SEVENZA_VERSION_MINOR}.zip)
					set(SEVENZA_MD5           2fac454a90ae96021f4ffc607d4c00f8)
				endif()
				require_definitions(SEVENZA_DIR SEVENZA_URL SEVENZA_MD5)
				if(NOT EXISTS "${SEVENZA_DIR}")
					file(MAKE_DIRECTORY "${SEVENZA_DIR}")
				endif()
				download_as_needed("${SEVENZA_URL}" "${SEVENZA_MD5}")
				extract_download("${SEVENZA_URL}" "${SEVENZA_DIR}")
				set(ENV{PATH} "$ENV{PATH};${SEVENZA_DIR}")
				find_program(SEVENZA_PROGRAM 7za)
				if(SEVENZA_PROGRAM MATCHES NOTFOUND)
					message(FATAL_ERROR "7za setup has finished, but 7za still cannot be found from the PATH" "$ENV{PATH}")
				endif()
				make_stamp("${SEVENZA_DIR}" "${SEVENZA_URL}")
			
			endif()
		endif()
		
	endif()
	
	message(STATUS "Setting up MSYS")
	download_as_needed("${MSYS_URL}" "${MSYS_MD5}")
	extract_download("${MSYS_URL}" .)
	if(NOT EXISTS "${CMAKE_MAKE_PROGRAM}")
		message(FATAL_ERROR "MSYS setup has finished, but ${CMAKE_MAKE_PROGRAM} is still missing")
	endif()
	make_stamp(msys "${MSYS_URL}")
	
endif()

check_stamp(match msys "${MSYS_URL}")
if(NOT match)
	message(WARNING "MSYS version does not match the configuration's version")
endif()

message(STATUS "Bootstrapping MinGW-builds - done")

file(WRITE make.cmd "@setlocal\n@%ComSpec% /C \"${CMAKE_MAKE_PROGRAM}\" %*\n") # Resulting file didn't work with quotes
file(WRITE cmake.cmd "@\"${CMAKE_COMMAND}\" %*\n")

# Configure the build system

if($ENV{NUMBER_OF_PROCESSORS})
	math(EXPR BUILD_JOBS $ENV{NUMBER_OF_PROCESSORS}+2)
	set(DEFINE_BUILD_JOBS "-DBUILD_JOBS=${BUILD_JOBS}")
	message(${DEFINE_BUILD_JOBS})
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" "${CMAKE_CURRENT_LIST_DIR}" "-G${CMAKE_GENERATOR}" -U*_PROGRAM -DMINGW_CONFIGURATION=gcc-${MINGW_GCC_VERSION}-${MINGW_GCC_EXCEPTIONS}-${MINGW_GCC_THREADS}-${MINGW_GCC_ARCH}-rev${MINGW_GCC_REVISION} -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} -DMSYS_DIR=${MSYS_DIR} ${DEFINE_BUILD_JOBS}
  RESULT_VARIABLE CONFIGURE_ERROR
)
if(CONFIGURE_ERROR)
	message(FATAL_ERROR "Could not generate the build system.")
endif()

# End of script


message("\nTo adjust the number of parallel jobs, run\n  cmake . -DBUILD_JOBS=<number>")