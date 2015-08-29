#
#    Copyright 2012, 2013, 2014 Thomas Schöps
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
 
cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
cmake_policy(SET CMP0020 OLD)

include(FeatureSummary)

# Project declaration

project(Mapper)

set(Mapper_COPYRIGHT "(C) 2012-2015 OpenOrienteering developers")

# All version components must be integers.
# We use Mapper_VERSION_PATCH >= 99 to mark development builds.
set(Mapper_VERSION_MAJOR 0)
set(Mapper_VERSION_MINOR 5)
if(NOT Mapper_VERSION_PATCH)
	# This can be configured when running cmake
	set(Mapper_VERSION_PATCH 99)
endif()

set(MAPPER_VERSION_PRI "${PROJECT_SOURCE_DIR}/oo-mapper-version.pri")
file(READ "${MAPPER_VERSION_PRI}" current)
string(MD5 file_md5 "${current}")
set(output "# Generated in CMakeLists.txt, do not edit here.
Mapper_VERSION_MAJOR = ${Mapper_VERSION_MAJOR}
Mapper_VERSION_MINOR = ${Mapper_VERSION_MINOR}
Mapper_VERSION_PATCH = ${Mapper_VERSION_PATCH}
")
string(MD5 output_md5 "${output}")
if(NOT "${output_md5}" STREQUAL "${file_md5}")
	file(WRITE "${MAPPER_VERSION_PRI}" "${output}")
endif()

if(${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
	message(FATAL_ERROR "You must not use the source directory as build directory.")
endif()

# Build configuration options

if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
	  "Type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	  FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(APPLE)
	SET(Mapper_MACOSX_VERSION "10.7" CACHE STRING
	  "Minimum Mac OS X version to build for, recommended: 10.7")
endif()

option(Mapper_TRANSLATIONS_EMBEDDED "Embed all translations in the program executable")

option(Mapper_DEBUG_TRANSLATIONS "Debug missing translations" OFF)

set(Mapper_BUILD_CLIPPER_DEFAULT ON)
if(WIN32 OR APPLE)
	set(Mapper_BUILD_PROJ_DEFAULT ON)
	set(Mapper_BUILD_QT_DEFAULT ON)
	set(Mapper_BUILD_DOXYGEN_DEFAULT ON)
	set(Mapper_PACKAGE_LINGUIST_DEFAULT ON)
else()
	set(Mapper_BUILD_PROJ_DEFAULT OFF)
	set(Mapper_BUILD_QT_DEFAULT ON)
	set(Mapper_BUILD_DOXYGEN_DEFAULT OFF)
	set(Mapper_PACKAGE_LINGUIST_DEFAULT OFF)
endif()

option(Mapper_BUILD_CLIPPER "Build the Clipper package from source" ${Mapper_BUILD_CLIPPER_DEFAULT})

option(Mapper_BUILD_PROJ "Build the Proj library from source" ${Mapper_BUILD_PROJ_DEFAULT})

option(Mapper_BUILD_QT   "Build the Qt library from source" ${Mapper_BUILD_QT_DEFAULT})

option(Mapper_BUILD_DOXYGEN "Build the doxygen tool from source" ${Mapper_BUILD_DOXYGEN_DEFAULT})

if(CMAKE_BUILD_TYPE MATCHES Release|MinSizeRel|RelWithDebInfo)
	set(Mapper_DEVELOPMENT_BUILD_DEFAULT OFF)
else()
	set(Mapper_DEVELOPMENT_BUILD_DEFAULT ON)
endif()
option(Mapper_DEVELOPMENT_BUILD "Configure development build (loading resource from the build directory)" ${Mapper_DEVELOPMENT_BUILD_DEFAULT})
mark_as_advanced(Mapper_DEVELOPMENT_BUILD)

option(Mapper_AUTORUN_UNIT_TESTS "Run the unit tests as part of the default target"   ${Mapper_DEVELOPMENT_BUILD})
option(Mapper_AUTORUN_SYSTEM_TESTS "Run the system tests as part of the default target" OFF)
mark_as_advanced(Mapper_AUTORUN_UNIT_TESTS Mapper_AUTORUN_SYSTEM_TESTS)

option(Mapper_BUILD_PACKAGE     "Create a target for building packages" ON)
option(Mapper_PACKAGE_PROJ      "Include all required Proj components in the packages" ${Mapper_BUILD_PROJ})
option(Mapper_PACKAGE_QT        "Include all required Qt components in the packages"   ${Mapper_BUILD_QT})
option(Mapper_PACKAGE_ASSISTANT "Include Qt Assistant in the packages"                 ${Mapper_BUILD_QT})
option(Mapper_PACKAGE_LINGUIST  "Build a Qt Linguist package"                          ${Mapper_PACKAGE_LINGUIST_DEFAULT})
mark_as_advanced(Mapper_BUILD_PACKAGE Mapper_PACKAGE_QT Mapper_PACKAGE_ASSISTANT Mapper_PACKAGE_PROJ Mapper_PACKAGE_LINGUIST)

unset(Mapper_PACKAGE_LIBRARIES CACHE) # Legacy


# C++11

# This option will be removed as soon as Mapper actually requires C++11.
option(Mapper_ENABLE_CXX11 "Enable C++11 support" ON)
mark_as_advanced(Mapper_ENABLE_CXX11)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" std_cxx11)
check_cxx_compiler_flag("-std=c++0x" std_cxx0x)
if(NOT Mapper_ENABLE_CXX11)
	message(WARNING "Compiler C++11 support (${std_cxx11}/${std_cxx0x}) is not enforced.")
elseif(std_cxx11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(std_cxx0x)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
elseif(CMAKE_COMPILER_IS_GNUXX)
	message(FATAL_ERROR "The compiler does not support C++11.")
else()
	message(FATAL_ERROR "The compiler support for C++11 is not known.")
endif()


# Installation configuration

# These value are used for the DESTINATION parameter of the install command
# and must not be empty.
if(WIN32)
	set(MAPPER_RUNTIME_DESTINATION .)
	set(MAPPER_LIBRARY_DESTINATION .)
	set(MAPPER_DATA_DESTINATION    .)
	set(MAPPER_ABOUT_DESTINATION   "doc")
elseif(APPLE)
	set(MAPPER_RUNTIME_DESTINATION .)
	set(MAPPER_LIBRARY_DESTINATION .)
	set(MAPPER_DATA_DESTINATION    "Mapper.app/Contents/Resources")
	set(MAPPER_ABOUT_DESTINATION   "Mapper.app/Contents/Resources")
else() # LINUX and alike
	set(Mapper_PACKAGE_NAME        "openorienteering-mapper" CACHE STRING
	  "The package name"
	)
	set(MAPPER_RUNTIME_DESTINATION "/usr/bin")
	set(MAPPER_LIBRARY_DESTINATION "/usr/lib/${Mapper_PACKAGE_NAME}")
	set(MAPPER_DATA_DESTINATION    "/usr/share/${Mapper_PACKAGE_NAME}")
	set(MAPPER_ABOUT_DESTINATION   "/usr/share/doc/${Mapper_PACKAGE_NAME}")
endif()

if(CMAKE_CROSSCOMPILING)
	message(STATUS "Crosscompiling, host: ${CMAKE_HOST_SYSTEM_NAME}, target: ${CMAKE_SYSTEM_NAME}")
endif()

# Build definitons

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(APPLE AND Mapper_MACOSX_VERSION)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=${Mapper_MACOSX_VERSION}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=${Mapper_MACOSX_VERSION}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mmacosx-version-min=${Mapper_MACOSX_VERSION}")
endif()

add_custom_target(Mapper_prerequisites)
set(Mapper_prerequisites_FOUND TRUE)

if(Mapper_BUILD_CLIPPER)
	add_subdirectory(3rd-party/clipper)
	add_dependencies(Mapper_prerequisites ${CLIPPER_LIBRARY})
	set_package_properties(Clipper PROPERTIES
	  DESCRIPTION "build from source, version: ${CLIPPER_VERSION}"
	)
endif()
find_package(Clipper MODULE REQUIRED)

if(Mapper_BUILD_PROJ)
	add_subdirectory(3rd-party/proj)
	add_dependencies(Mapper_prerequisites Proj)
	if(NOT PROJ_FOUND)
		set(Mapper_prerequisites_FOUND FALSE)
		message(STATUS "Proj library will be built from source")
	endif()
else()
	find_package(Proj REQUIRED)
endif()

if(Mapper_BUILD_QT)
	add_subdirectory(3rd-party/qt5)
	add_dependencies(Mapper_prerequisites Qt5)
	list(INSERT CMAKE_PREFIX_PATH 0 "${QT5_DIR}")
	find_package(Qt5Core QUIET PATHS "${QT5_DIR}" NO_DEFAULT_PATH)
	if(NOT Qt5Core_FOUND)
		set(Mapper_prerequisites_FOUND FALSE)
		message(STATUS "Qt5 library will be built from source")
	endif()
else()
	# may require -DCMAKE_PREFIX_PATH=/path-to-qt5-dir
	find_package(Qt5Core REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")
endif()

if(Mapper_BUILD_DOXYGEN)
	add_subdirectory(3rd-party/doxygen)
	add_feature_info(doxygen-project 1 "build from source, version: ${DOXYGEN_VERSION}")
endif()

if(Mapper_prerequisites_FOUND)

foreach(qt5_module Qt5Core Qt5Gui Qt5Widgets Qt5Xml Qt5Network Qt5PrintSupport)
	find_package(${qt5_module} REQUIRED)
	add_definitions(${${qt5_module}_DEFINITIONS})
	list(APPEND QT_INCLUDES ${${qt5_module}_INCLUDE_DIRS})
	list(APPEND QT_LIBRARIES ${${qt5_module}_LIBRARIES})
endforeach()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")
if(Mapper_DEVELOPMENT_BUILD)
	foreach(qt5_module Qt5Test)
		find_package(${qt5_module} REQUIRED)
		add_definitions(${${qt5_module}_DEFINITIONS})
		list(APPEND QT_INCLUDES ${${qt5_module}_INCLUDE_DIRS})
		list(APPEND QT_LIBRARIES ${${qt5_module}_LIBRARIES})
	endforeach()
endif()
# FIXME Get rid of all dependencies on private headers!
if(WIN32)
	foreach(qt5_module Qt5Core Qt5Gui Qt5PrintSupport)
		find_package(${qt5_module} REQUIRED)
		if("${${qt5_module}_PRIVATE_INCLUDE_DIRS}" STREQUAL "")
			# While "find_package(Qt5Xyz REQUIRED COMPONENTS Private)" is supposed to set
			# Qt5Xyz_PRIVATE_INCLUDE_DIRS, it doesn't seem to take place in Qt 5.2.0
			# Cf. https://bugreports.qt-project.org/browse/QTBUG-37417
			foreach(base_dir ${${qt5_module}_INCLUDE_DIRS})
				string(REPLACE Qt5 Qt qt_module ${qt5_module})
				if("${base_dir}" MATCHES "/${qt_module}\$")
					list(APPEND ${qt5_module}_PRIVATE_INCLUDE_DIRS "${base_dir}/${${qt5_module}_VERSION}/${qt_module}")
				endif()
			endforeach()
		endif()
		list(APPEND QT_INCLUDES ${${qt5_module}_PRIVATE_INCLUDE_DIRS})
	endforeach()
endif()
list(REMOVE_DUPLICATES QT_INCLUDES)
list(REMOVE_DUPLICATES QT_LIBRARIES)
list(REVERSE QT_LIBRARIES)

if(UNIX AND NOT APPLE)
#	set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib/${Mapper_PACKAGE_NAME}/lib")
	set(CMAKE_INSTALL_RPATH "${MAPPER_LIBRARY_DESTINATION}/lib")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wpedantic")
endif()

add_definitions(-D_USE_MATH_DEFINES -DUNICODE)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${PROJ_INCLUDE_DIR})

if(Mapper_DEVELOPMENT_BUILD)
	add_definitions(-DMAPPER_DEVELOPMENT_BUILD)
else()
	add_definitions(-DQT_NO_DEBUG -DQT_NO_DEBUG_OUTPUT -DQT_NO_WARNING_OUTPUT -DNDEBUG)
endif()

if(NOT Mapper_VERSION_PATCH LESS 99)
	set(Mapper_VERSION_DISPLAY "unstable ${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}")
else()
	set(Mapper_VERSION_DISPLAY "${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}")
endif()

# Subdirectories

add_subdirectory("docs/licensing")
add_subdirectory("docs/manual")
add_subdirectory("examples")
add_subdirectory("symbol sets")
add_subdirectory("translations")
add_subdirectory("3rd-party/qbezier")
get_property(_definitions DIRECTORY PROPERTY COMPILE_DEFINITIONS)
if(_definitions MATCHES "QT_NETWORK_LIB")
	add_subdirectory("3rd-party/qtsingleapplication")
endif()
add_subdirectory("src/libocad")
add_subdirectory("src")
if(Mapper_BUILD_PACKAGE)
	add_subdirectory("packaging")
endif()
if(CMAKE_CROSSCOMPILING)
	add_custom_target(TEST_WARNING ALL
	  COMMENT "Crosscompiling, skipping all tests")
	add_dependencies(TEST_WARNING Mapper)
else()
	enable_testing()
	add_subdirectory("test")
endif()

install(FILES COPYING 
  DESTINATION "${MAPPER_ABOUT_DESTINATION}"
)
configure_file(COPYING COPYING COPYONLY)

set(MAPPER_SUPERBUILD 0 CACHE INTERNAL "Flag to avoid superbuild loops." FORCE)

elseif(MAPPER_SUPERBUILD)

	set(MAPPER_SUPERBUILD 0 CACHE INTERNAL "Flag to avoid superbuild loops." FORCE)
	message(FATAL_ERROR "Mapper prerequisites marked as build but not found by cmake re-run.")

else() # NOT Mapper_prerequisites_FOUND AND NOT MAPPER_SUPERBUILD

	if("${CMAKE_GENERATOR}" MATCHES "Makefiles" OR 
	   "${CMAKE_EXTRA_GENERATOR}" MATCHES "Makefiles")
		# use magic MAKE macro
		set(_build_tool "\$(MAKE)")
	else()
		set(_build_tool ${CMAKE_BUILD_TOOL})
	endif()
	add_custom_target(Mapper_superbuild ALL
	  COMMAND "${CMAKE_COMMAND}" . -DMAPPER_SUPERBUILD=1
	  COMMAND ${_build_tool}
	  VERBATIM
	)
	add_dependencies(Mapper_superbuild Mapper_prerequisites)

endif()

add_subdirectory("docs/api")
add_subdirectory("packaging/src")

feature_summary(WHAT ALL)

string(TOUPPER "${CMAKE_BUILD_TYPE}" type)
foreach(lang CXX C)
	foreach(i "" _${type})
		message(STATUS "CMAKE_${lang}_FLAGS${i}: ${CMAKE_${lang}_FLAGS${i}}")
	endforeach()
endforeach()
