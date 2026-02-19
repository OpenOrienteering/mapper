#
#    Copyright 2012, 2013, 2014 Thomas Schöps
#    Copyright 2012-2026 Kai Pastor
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
 
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(POLICY CMP0115)
    cmake_policy(SET CMP0115 NEW)
endif()
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FeatureSummary)

if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

option(CMAKE_FIND_PACKAGE_PREFER_CONFIG "Lookup package config files before using find modules" ON)

# Project declaration

project(Mapper VERSION 0.9.7 LANGUAGES CXX C)
set(Mapper_COPYRIGHT "(C) 2026 The OpenOrienteering developers")

math(EXPR Mapper_VERSION_CODE "${Mapper_VERSION_MAJOR} * 10000 + ${Mapper_VERSION_MINOR} * 100 + ${Mapper_VERSION_PATCH} * 2 + ${CMAKE_SIZEOF_VOID_P} / 4 - 1")

if(Mapper_VERSION_DISPLAY)
	message(STATUS "Custom version display string: \"${Mapper_VERSION_DISPLAY}\"")
elseif(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "Release|MinSizeRel|RelWithDebInfo")
	set(Mapper_VERSION_DISPLAY "${CMAKE_BUILD_TYPE} ${Mapper_VERSION}")
else()
	set(Mapper_VERSION_DISPLAY "${Mapper_VERSION}")
endif()

string(REGEX REPLACE "[\\/:*?| ]" "_" Mapper_VERSION_FILESYSTEM "${Mapper_VERSION_DISPLAY}")

if(${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
	message(AUTHOR_WARNING "In-source builds are discouraged for development.")
endif()

# Build configuration options

if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
	  "Type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	  FORCE)
endif(NOT CMAKE_BUILD_TYPE)

option(Mapper_DEBUG_TRANSLATIONS "Debug missing translations" OFF)

# To improve developer experience, build clipper if it is not found
set(Mapper_BUILD_CLIPPER "auto" CACHE STRING
  "Build the Clipper package from source, alternatives: ON, OFF"
)

option(Mapper_USE_GDAL   "Use the GDAL library" ON)

if(ANDROID)
	set(Mapper_WITH_COVE_DEFAULT OFF)
else()
	set(Mapper_WITH_COVE_DEFAULT ON)
endif()
option(Mapper_WITH_COVE "Build and include contour line vectorization" ${Mapper_WITH_COVE_DEFAULT})

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
	include(GNUInstallDirs)
	set(MAPPER_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
	set(MAPPER_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}/${Mapper_PACKAGE_NAME}")
	set(MAPPER_DATA_DESTINATION    "${CMAKE_INSTALL_DATADIR}/${Mapper_PACKAGE_NAME}")
	string(REPLACE "/${PROJECT_NAME}" "/${Mapper_PACKAGE_NAME}"
	    MAPPER_ABOUT_DESTINATION   "${CMAKE_INSTALL_DOCDIR}")
endif()

if(CMAKE_CROSSCOMPILING)
	message(STATUS "Crosscompiling, host: ${CMAKE_HOST_SYSTEM_NAME}, target: ${CMAKE_SYSTEM_NAME}")
endif()


# Build definitions

include(CTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

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

if(Mapper_BUILD_CLIPPER STREQUAL "auto")
	find_package(Polyclipping 6.1.3 MODULE)
	if(NOT Polyclipping_FOUND)
		message(WARNING
		  "System polyclipping is missing. Enabling embedded build.\n"
		  "Set Mapper_BUILD_CLIPPER=OFF to disable embedded build."
		)
		set_property(CACHE Mapper_BUILD_CLIPPER PROPERTY VALUE "ON")
	else()
		set_property(CACHE Mapper_BUILD_CLIPPER PROPERTY VALUE "OFF")
	endif()
	set_property(CACHE Mapper_BUILD_CLIPPER PROPERTY TYPE "BOOL")
endif()
if(Mapper_BUILD_CLIPPER)
	add_subdirectory(3rd-party/clipper)
	add_feature_info(Mapper_BUILD_CLIPPER 1 "version: ${CLIPPER_VERSION}")
	add_library(Polyclipping::Polyclipping ALIAS polyclipping)
else()
	find_package(Polyclipping 6.1.3 MODULE REQUIRED)
endif()

# We must not require a minimum version of PROJ via find_package
# because PROJ config requires the major version to match exactly.
find_package(PROJ CONFIG)
if(NOT PROJ_LIBRARIES)
	find_package(PkgConfig REQUIRED)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(PROJ4_PC IMPORTED_TARGET proj)
	endif()
	if(TARGET PkgConfig::PROJ4_PC)
		set(PROJ_LIBRARIES PkgConfig::PROJ4_PC)
		set(PROJ_VERSION "${PROJ4_PC_VERSION}")
	endif()
endif()
if(NOT PROJ_VERSION OR PROJ_VERSION VERSION_LESS 4.9)
	message(FATAL_ERROR "At least PROJ 4.9 is required")
elseif(PROJ_VERSION VERSION_LESS 6.1)
	# New PROJ API missing or incomplete.
	# (proj_normalize_for_visualization() came in 6.1.)
	set_property(TARGET "${PROJ_LIBRARIES}" APPEND PROPERTY
	  INTERFACE_COMPILE_DEFINITIONS ACCEPT_USE_OF_DEPRECATED_PROJ_API_H)
elseif(PROJ_VERSION VERSION_LESS 6.2.1)
	# Datum Potsdam issue, https://github.com/OSGeo/PROJ/pull/1573
	set_property(TARGET "${PROJ_LIBRARIES}" APPEND PROPERTY
	  INTERFACE_COMPILE_DEFINITIONS PROJ_ISSUE_1573)
endif()

if(Mapper_USE_GDAL)
	find_package(GDAL MODULE REQUIRED)
endif()

find_package(Qt5Core 5.5 REQUIRED)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050500)
if (ANDROID AND "${Qt5Core_VERSION}" VERSION_LESS 5.12.1)
	message(FATAL_ERROR "At least Qt 5.12.1 is required to build for Android")
elseif ("${Qt5Core_VERSION}" VERSION_LESS 5.8.0)
	add_definitions("-D'Q_FALLTHROUGH()=(void)0'")
endif()

include("TestBigEndian")
test_big_endian(big_endian)
if(big_endian)
	add_definitions(-DMAPPER_BIG_ENDIAN)
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


# Build-time code checks

if(NOT CMAKE_HOST_WIN32)
	find_program(SH_EXECUTABLE NAMES sh PATHS ENV PATH NO_DEFAULT_PATH)
endif()

if(NOT CMAKE_CXX_CLANG_TIDY AND Mapper_DEVELOPMENT_BUILD AND SH_EXECUTABLE)
	set(min_version 8)
	find_package(ClangTidy ${min_version})
	if(ClangTidy_FOUND)
		set(CMAKE_CXX_CLANG_TIDY "${SH_EXECUTABLE};-e;${PROJECT_SOURCE_DIR}/code-check-wrapper.sh;${ClangTidy_EXECUTABLE}")
	else()
		message(AUTHOR_WARNING "Install clang-tidy >= ${min_version} for code checks during build")
	endif()
endif()

if(NOT CMAKE_CXX_INCLUDE_WHAT_YOU_USE AND Mapper_DEVELOPMENT_BUILD AND SH_EXECUTABLE)
	set(min_version 0.12)
	find_package(IWYU ${min_version})
	if(IWYU_FOUND)
		set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${SH_EXECUTABLE};-e;${PROJECT_SOURCE_DIR}/code-check-wrapper.sh;${IWYU_EXECUTABLE}")
	else()
		message(AUTHOR_WARNING "Install include-what-you-use >= ${min_version} for code checks during build")
	endif()
endif()


# Subdirectories

add_subdirectory("doc/manual")
add_subdirectory("examples")
add_subdirectory("symbol sets")
add_subdirectory("translations")
if(Mapper_WITH_COVE)
	add_feature_info(Mapper_WITH_COVE "${Mapper_WITH_COVE}" "Contour line vectorization")
	add_subdirectory("3rd-party/cove")
endif()
if(NOT ANDROID)
	add_subdirectory("3rd-party/qtsingleapplication")
endif()
if (Mapper_USE_GDAL)
	add_subdirectory("src/gdal")
endif()
if(NOT ANDROID)
	add_subdirectory("src/printsupport")
endif()
add_subdirectory("src/sensors")
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

# Auxiliary targets

set(ci
  ci/azure-pipelines.yml
  ci/build.yml
  ci/codespell.yml
  ci/filter-stderr.sed
  ci/publish.yml
  ci/publish-coverage.yml
  ci/release-notes.yml
  ci/setup-common.yml
  ci/setup-macos.yml
  ci/setup-msys2.yml
  ci/setup-ubuntu.yml
  ci/shell.sh
  ci/openorienteering-mapper-ci.cmake
)

add_custom_target(CI SOURCES ${ci})

# Configuration report

feature_summary(INCLUDE_QUIET_PACKAGES WHAT ALL)

string(TOUPPER "${CMAKE_BUILD_TYPE}" type)
foreach(lang CXX C)
	foreach(i "" _${type})
		message(STATUS "CMAKE_${lang}_FLAGS${i}: ${CMAKE_${lang}_FLAGS${i}}")
	endforeach()
endforeach()
