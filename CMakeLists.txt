#
#    Copyright 2012, 2013, 2014 Thomas Schöps
#    Copyright 2012-2018 Kai Pastor
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
 
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FeatureSummary)

if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Project declaration

project(Mapper VERSION 0.8.2 LANGUAGES CXX C)

if(Mapper_VERSION_DISPLAY)
	message(STATUS "Custom version display string: \"${Mapper_VERSION_DISPLAY}\"")
elseif(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "Release|MinSizeRel|RelWithDebInfo")
	set(Mapper_VERSION_DISPLAY "${CMAKE_BUILD_TYPE} ${Mapper_VERSION}")
else()
	set(Mapper_VERSION_DISPLAY "${Mapper_VERSION}")
endif()

set(Mapper_COPYRIGHT "(C) 2012-2018 The OpenOrienteering developers")

set(android_manifest "${PROJECT_SOURCE_DIR}/android/AndroidManifest.xml")
file(READ "${android_manifest}" current)
if(NOT current MATCHES " android:versionName=.${Mapper_VERSION_MAJOR}\\.${Mapper_VERSION_MINOR}\\.${Mapper_VERSION_PATCH}. ")
	set(android_version_name "${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}")
	math(EXPR android_version_int "${Mapper_VERSION_MAJOR} * 10000 + ${Mapper_VERSION_MINOR} * 100 + ${Mapper_VERSION_PATCH}")
	string(REGEX REPLACE "( android:versionName=.)[0-9]*\\.[0-9]*\\.[0-9]*(. )" "\\1${android_version_name}\\2" output "${current}")
	string(REGEX REPLACE "( android:versionCode=.)[0-9]*(. )" "\\1${android_version_int}\\2" output "${output}")
	file(WRITE "${android_manifest}" "${output}")
endif()

if(${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
	message(AUTHOR_WARNING "In-source builds are discouraged for development.")
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

option(Mapper_DEBUG_TRANSLATIONS "Debug missing translations" OFF)

# Used for some Linux distributions which do not provide the polyclipping lib.
option(Mapper_BUILD_CLIPPER "Build the Clipper package from source" OFF)

option(Mapper_USE_GDAL   "Use the GDAL library" ON)

if(CMAKE_BUILD_TYPE MATCHES Release|MinSizeRel|RelWithDebInfo)
	set(Mapper_DEVELOPMENT_BUILD_DEFAULT OFF)
else()
	set(Mapper_DEVELOPMENT_BUILD_DEFAULT ON)
endif()
option(Mapper_DEVELOPMENT_BUILD "Configure development build (loading resource from the build directory)" ${Mapper_DEVELOPMENT_BUILD_DEFAULT})
mark_as_advanced(Mapper_DEVELOPMENT_BUILD)

option(Mapper_AUTORUN_SYSTEM_TESTS "Run the system tests as part of the Mapper_Test target" ${Mapper_DEVELOPMENT_BUILD})
option(Mapper_AUTORUN_MANUAL_TESTS "Run the system tests as part of the Mapper_Test target" OFF)
mark_as_advanced(Mapper_AUTORUN_SYSTEM_TESTS Mapper_AUTORUN_MANUAL_TESTS)

if(ANDROID OR APPLE OR WIN32)
	set(mapper_package_default ON)
else()
	set(mapper_package_default OFF)
endif()
if(Mapper_USE_GDAL)
	set(mapper_package_gdal ${mapper_package_default})
else()
	set(mapper_package_gdal OFF)
endif()
if(NOT ANDROID)
	set(mapper_package_assistant ${mapper_package_default})
else()
	set(mapper_package_assistant OFF)
endif()
option(Mapper_PACKAGE_PROJ      "Include all required Proj components in the packages" ${mapper_package_default})
option(Mapper_PACKAGE_GDAL      "Include all required GDAL components in the packages" ${mapper_package_gdal})
option(Mapper_PACKAGE_QT        "Include all required Qt components in the packages"   ${mapper_package_default})
option(Mapper_PACKAGE_ASSISTANT "Include Qt Assistant in the packages"                 ${mapper_package_assistant})
mark_as_advanced(Mapper_PACKAGE_PROJ Mapper_PACKAGE_GDAL Mapper_PACKAGE_QT Mapper_PACKAGE_ASSISTANT)


# Migration of legacy configurations (before 0.7, affects developers)

if(Mapper_BUILD_DOXYGEN)
	unset(DOXYGEN_EXECUTABLE CACHE)
endif()
unset(Mapper_BUILD_DOXYGEN CACHE)
unset(Mapper_BUILD_GDAL CACHE)
unset(Mapper_BUILD_PROJ CACHE)
unset(Mapper_BUILD_QT CACHE)


# Installation configuration

set(Mapper_PACKAGE_NAME "openorienteering-mapper" CACHE STRING
  "The package name"
)

# These value are used for the DESTINATION parameter of the install command
# and must not be empty.
if(WIN32 AND BIN_INSTALL_DIR)
	set(MAPPER_RUNTIME_DESTINATION "${BIN_INSTALL_DIR}")
	set(MAPPER_LIBRARY_DESTINATION "${LIB_INSTALL_DIR}")
	set(MAPPER_DATA_DESTINATION    "${SHARE_INSTALL_DIR}/${Mapper_PACKAGE_NAME}")
	set(MAPPER_ABOUT_DESTINATION   "${SHARE_INSTALL_DIR}/doc/${Mapper_PACKAGE_NAME}")
elseif(WIN32)
	set(MAPPER_RUNTIME_DESTINATION .)
	set(MAPPER_LIBRARY_DESTINATION .)
	set(MAPPER_DATA_DESTINATION    .)
	set(MAPPER_ABOUT_DESTINATION   "doc")
elseif(APPLE)
	set(MAPPER_RUNTIME_DESTINATION .)
	set(MAPPER_LIBRARY_DESTINATION .)
	set(MAPPER_DATA_DESTINATION    "Mapper.app/Contents/Resources")
	set(MAPPER_ABOUT_DESTINATION   "Mapper.app/Contents/Resources/doc")
elseif(ANDROID)
	set(MAPPER_RUNTIME_DESTINATION "libs/${CMAKE_ANDROID_ARCH_ABI}")
	set(MAPPER_LIBRARY_DESTINATION "libs/${CMAKE_ANDROID_ARCH_ABI}")
	set(MAPPER_DATA_DESTINATION    "assets")
	set(MAPPER_ABOUT_DESTINATION   "assets/doc")
else() # LINUX and alike
	set(MAPPER_RUNTIME_DESTINATION "bin")
	set(MAPPER_LIBRARY_DESTINATION "lib/${Mapper_PACKAGE_NAME}")
	set(MAPPER_DATA_DESTINATION    "share/${Mapper_PACKAGE_NAME}")
	set(MAPPER_ABOUT_DESTINATION   "share/doc/${Mapper_PACKAGE_NAME}")
endif()

if(CMAKE_CROSSCOMPILING)
	message(STATUS "Crosscompiling, host: ${CMAKE_HOST_SYSTEM_NAME}, target: ${CMAKE_SYSTEM_NAME}")
endif()


# Build definitons

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(APPLE AND Mapper_MACOSX_VERSION)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -mmacosx-version-min=${Mapper_MACOSX_VERSION}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=${Mapper_MACOSX_VERSION}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mmacosx-version-min=${Mapper_MACOSX_VERSION}")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wpedantic")
endif()

add_custom_target(Mapper_prerequisites
  SOURCES # Extra files to be shown in IDE
    INSTALL.md
    README.md
)
set(Mapper_prerequisites_FOUND TRUE)

if(Mapper_BUILD_CLIPPER)
	add_subdirectory(3rd-party/clipper)
	add_feature_info(Mapper_BUILD_CLIPPER 1 "version: ${CLIPPER_VERSION}")
	add_library(Polyclipping::Polyclipping ALIAS polyclipping)
else()
	find_package(Polyclipping 6.1.3 MODULE REQUIRED)
endif()

find_package(PROJ4 CONFIG QUIET)
if(NOT TARGET PROJ4::proj)
	set(PROJ4_FOUND false)
	find_package(PROJ4 MODULE REQUIRED)
endif()

if(Mapper_USE_GDAL)
	find_package(GDAL REQUIRED)
endif()


include("TestBigEndian")
test_big_endian(big_endian)
if(big_endian)
	add_definitions(-DMAPPER_BIG_ENDIAN)
endif()


if(ANDROID OR big_endian)
	add_definitions(-DNO_NATIVE_FILE_FORMAT)
endif()

if(UNIX AND NOT APPLE)
#	set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib/${Mapper_PACKAGE_NAME}/lib")
	set(CMAKE_INSTALL_RPATH "${MAPPER_LIBRARY_DESTINATION}/lib")
endif()

add_definitions(-D_USE_MATH_DEFINES -DUNICODE)

if(Mapper_DEVELOPMENT_BUILD)
	add_definitions(-DMAPPER_DEVELOPMENT_BUILD)
	include(EnableSanitize)
	enable_sanitize(address undefined)
	configure_file(suppress.txt.in suppress.txt COPYONLY)
else()
	add_definitions(-DQT_NO_DEBUG -DQT_NO_DEBUG_OUTPUT -DQT_NO_WARNING_OUTPUT -DNDEBUG)
endif()

# Subdirectories

add_subdirectory("doc/manual")
add_subdirectory("examples")
add_subdirectory("symbol sets")
add_subdirectory("translations")
add_subdirectory("3rd-party/qbezier")
if(NOT ANDROID)
	add_subdirectory("3rd-party/qtsingleapplication")
endif()
if (Mapper_USE_GDAL)
	add_subdirectory("src/gdal")
endif()
add_subdirectory("src/libocad")
if(NOT ANDROID)
	add_subdirectory("src/printsupport")
endif()
add_subdirectory("src")
add_subdirectory("packaging")
add_subdirectory("doc/licensing")

if(CMAKE_CROSSCOMPILING)
	add_custom_target(TEST_WARNING ALL
	  COMMENT "Crosscompiling, skipping all tests")
	add_dependencies(TEST_WARNING Mapper)
else()
	enable_testing()
	add_subdirectory("test")
endif()

add_subdirectory("doc/api")
add_subdirectory("packaging/src")

feature_summary(WHAT ALL)

string(TOUPPER "${CMAKE_BUILD_TYPE}" type)
foreach(lang CXX C)
	foreach(i "" _${type})
		message(STATUS "CMAKE_${lang}_FLAGS${i}: ${CMAKE_${lang}_FLAGS${i}}")
	endforeach()
endforeach()
