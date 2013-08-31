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


project(MinGW NONE)

cmake_minimum_required(VERSION 2.8.3)

include("${CMAKE_CURRENT_LIST_DIR}/functions.cmake")


# Configuration options

set(MINGW_CONFIGURATION_DEFAULT x32-4.8.0-release-posix-dwarf-rev2-openorienteering)
set(MINGW_CONFIGURATION ${MINGW_CONFIGURATION_DEFAULT} CACHE STRING
  "MinGW configuration name, suggested value: ${MINGW_CONFIGURATION_DEFAULT}")

set(MSYS_DIR "C:/msys" CACHE PATH
  "Path to the MSYS root directory")

set(BUILD_JOBS 1 CACHE STRING
  "Number of build jobs, recommended value: Number of CPUs + 1")

set(KEEP_GOING 0 CACHE BOOL
  "Retry builds until success or canceled")


# Configuration checking

message(STATUS "Configuring MinGW ${MINGW_CONFIGURATION}")

find_program(SH_PROGRAM sh
  PATHS ${MSYS_DIR}/bin
  NO_DEFAULT_PATH)
mark_as_advanced(SH_PROGRAM)

if(SH_PROGRAM MATCHES NOTFOUND)
	message(FATAL_ERROR
	  "Couldn't find sh executable. "
	  "Specify the correct MSYS_DIR path at the command line.")
endif()

if (NOT ${MINGW_CONFIGURATION} STREQUAL ${MINGW_CONFIGURATION_DEFAULT})
	message(WARNING
	  "The MinGW version is different from the current recommended version "
	  "(${MINGW_CONFIGURATION} vs. ${MINGW_CONFIGURATION_DEFAULT}).")
endif()

set(MINGW_CONFIGURATION_FILE "${PROJECT_SOURCE_DIR}/configurations/${MINGW_CONFIGURATION}.cmake")

message(STATUS "Checking configuration:")
if(NOT EXISTS "${MINGW_CONFIGURATION_FILE}")
	message(FATAL_ERROR "No such file: ${MINGW_CONFIGURATION_FILE}")
else()
	message("   Path: ${MINGW_CONFIGURATION_FILE} - found")
endif()


# Configuration loading

include(${MINGW_CONFIGURATION_FILE})

set(MINGW_BUILDROOT "${MSYS_DIR}/temp")
set(CONTROL_DIR "${CMAKE_CURRENT_BINARY_DIR}/control")

message("   Definitions:")
require_definitions(
  MINGW_CONFIGURATION_NAME
  MINGW_SOURCES_URL
  MINGW_SOURCES_MD5
  MINGW_BUILDS_URL
  MINGW_BUILDS_MD5
  MINGW_GCC_VERSION
  MINGW_GCC_REVISION
  MINGW_GCC_EXCEPTIONS
  MINGW_GCC_THREADS
  MINGW_GCC_ARCH
  MINGW_TOOLCHAIN_VERSION
  MINGW_TOOLCHAIN_MD5
  MINGW_TOOLCHAIN_URL
  MINGW_BUILDROOT
)

file(MAKE_DIRECTORY "${MINGW_BUILDROOT}")
file(MAKE_DIRECTORY "${MINGW_BUILDROOT}/mingw-builds")

# downloading and extracting the sources
configure_file(preload-sources.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/control/preload-sources.cmake.new @ONLY)
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different preload-sources.cmake.new preload-sources.cmake
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/control"
  RESULT_VARIABLE COPY_ERROR
)
if(COPY_ERROR)
	message(FATAL_ERROR "Could not update the sources configuration")
endif()
add_custom_target(preload-sources
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/control/preload-sources.cmake
          ${MINGW_BUILDROOT}
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/control/preload-sources.cmake
  WORKING_DIRECTORY "${MINGW_BUILDROOT}"
  VERBATIM
)



# downloading and extracting mingw-builds
configure_file(preload-mingw-builds.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/control/preload-mingw-builds.cmake.new @ONLY)
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different preload-mingw-builds.cmake.new preload-mingw-builds.cmake
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/control"
  RESULT_VARIABLE COPY_ERROR
)
if(COPY_ERROR)
	message(FATAL_ERROR "Could not update the mingw-builds configuration")
endif()
add_custom_command(
  OUTPUT ${MINGW_BUILDROOT}/mingw-builds/build ${MINGW_BUILDROOT}/mingw-builds/README.OpenOrienteering.txt
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/control/preload-mingw-builds.cmake
  WORKING_DIRECTORY "${MINGW_BUILDROOT}"
  VERBATIM
)
add_custom_target(preload-mingw-builds
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/control/preload-mingw-builds.cmake
          ${MINGW_BUILDROOT}
          ${MINGW_BUILDROOT}/mingw-builds/build
)



# downloading and extracting the toolchain
configure_file(preload-toolchain.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/control/preload-toolchain.cmake.new @ONLY)
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different preload-toolchain.cmake.new preload-toolchain.cmake
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/control"
  RESULT_VARIABLE COPY_ERROR
)
if(COPY_ERROR)
	message(FATAL_ERROR "Could not update the toolchain configuration")
endif()
add_custom_target(preload-toolchain
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/control/preload-toolchain.cmake
          ${MINGW_BUILDROOT}/mingw-builds
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/control/preload-toolchain.cmake
  WORKING_DIRECTORY "${MINGW_BUILDROOT}/mingw-builds"
  VERBATIM
)



# build mingw
configure_file(run-mingw-builds.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/control/run-mingw-builds.cmake.new @ONLY)
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different run-mingw-builds.cmake.new run-mingw-builds.cmake
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/control"
  RESULT_VARIABLE COPY_ERROR
)
if(COPY_ERROR)
	message(FATAL_ERROR "Could not update the build configuration")
endif()
add_custom_target(OUTPUT ${MINGW_BUILDROOT}/info-stamp.txt
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/control/run-mingw-builds.cmake ${MINGW_BUILDROOT}/src/info-stamp.txt ${MINGW_BUILDROOT}/mingw-builds/toolchains/info-stamp.txt
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/control/run-mingw-builds.cmake
  WORKING_DIRECTORY "${MINGW_BUILDROOT}/mingw-builds"
  VERBATIM
)



# General
add_custom_target(scratch
  COMMAND ${CMAKE_COMMAND} -E remove_directory "${MINGW_BUILDROOT}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${MINGW_BUILDROOT}/mingw-builds"
  VERBATIM
)

add_custom_target(preload-tools
  DEPENDS preload-mingw-builds preload-toolchain
)

add_custom_target(download
  DEPENDS ${MSYS_DIR}/bin ${MINGW_BUILDROOT}/mingw-builds/build
  COMMAND ${CMAKE_COMMAND} -DBUILD_TARGET=download -P ${CMAKE_CURRENT_BINARY_DIR}/control/run-mingw-builds.cmake
  WORKING_DIRECTORY "${MINGW_BUILDROOT}/mingw-builds"
  VERBATIM
)

add_custom_target(build ALL
  DEPENDS ${MSYS_DIR}/bin ${MINGW_BUILDROOT}/mingw-builds/build download
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/control/run-mingw-builds.cmake
  WORKING_DIRECTORY "${MINGW_BUILDROOT}/mingw-builds"
  VERBATIM
)

add_custom_target(build-from-archive
  DEPENDS ${MSYS_DIR}/bin preload-tools preload-sources
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/control/run-mingw-builds.cmake
  VERBATIM
)

add_custom_target(build-from-upstream
  DEPENDS ${MSYS_DIR}/bin preload-tools
  COMMAND ${CMAKE_COMMAND} -DBUILD_TARGET=download -P ${CMAKE_CURRENT_BINARY_DIR}/control/run-mingw-builds.cmake
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/control/run-mingw-builds.cmake
  VERBATIM
)

add_custom_target(archives
  DEPENDS ${MSYS_DIR}/bin ${MINGW_BUILDROOT}/mingw-builds/build
  COMMAND ${CMAKE_COMMAND} -DBUILD_TARGET=archives -P ${CMAKE_CURRENT_BINARY_DIR}/control/run-mingw-builds.cmake
  WORKING_DIRECTORY "${MINGW_BUILDROOT}/mingw-builds"
  VERBATIM
)

set(NEW_MINGW_SOURCES_MD5 "@NEW_MINGW_SOURCES_MD5@") # Delay expansion
configure_file(configuration-template.cmake.in configuration-template.cmake.in @ONLY)

require_definitions(BUILD_JOBS)

# End of script
