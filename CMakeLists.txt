#
#    Copyright 2012 Thomas Schöps, Kai Pastor
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
 
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Project declaration

project(Mapper)

# All version components must be integers.
# We use 99 to mark development builds.
SET(Mapper_VERSION_MAJOR 0)
SET(Mapper_VERSION_MINOR 4)
SET(Mapper_VERSION_PATCH 99)

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
	SET(Mapper_MACOSX_VERSION "10.6" CACHE STRING
	  "Minimum Mac OS X version to build for, recommended: 10.6")
endif()

option(Mapper_TRANSLATIONS_EMBEDDED "Embed all translations in the program executable")

if(WIN32 OR APPLE)
	set(Mapper_BUILD_PROJ_DEFAULT ON)
	set(Mapper_BUILD_QT_DEFAULT ON)
else()
	set(Mapper_BUILD_PROJ_DEFAULT OFF)
	set(Mapper_BUILD_QT_DEFAULT ON)
endif()

option(Mapper_BUILD_PROJ "Build the Proj library from source" ${Mapper_BUILD_PROJ_DEFAULT})

option(Mapper_BUILD_QT   "Build the Qt library from source" ${Mapper_BUILD_QT_DEFAULT})

option(Mapper_USE_QT5    "Use Qt library version 5" ${Mapper_BUILD_QT})

if(NOT CMAKE_BUILD_TYPE MATCHES Release|MinSizeRel)
	set(Mapper_DEVELOPMENT_BUILD_DEFAULT ON)
else()
	set(Mapper_DEVELOPMENT_BUILD_DEFAULT OFF)
endif()
option(Mapper_DEVELOPMENT_BUILD "Configure development build (loading resource from the build directory)" ${Mapper_DEVELOPMENT_BUILD_DEFAULT})
mark_as_advanced(Mapper_DEVELOPMENT_BUILD)

option(Mapper_AUTORUN_UNIT_TESTS "Run the unit tests as part of the default target"   ON)
option(Mapper_AUTORUN_SYSTEM_TESTS "Run the system tests as part of the default target" OFF)
mark_as_advanced(Mapper_AUTORUN_UNIT_TESTS Mapper_AUTORUN_SYSTEM_TESTS)

option(Mapper_BUILD_PACKAGE     "Create a target for building packages" ON)
option(Mapper_PACKAGE_PROJ      "Include all required Proj components in the packages" ${Mapper_BUILD_PROJ})
option(Mapper_PACKAGE_QT        "Include all required Qt components in the packages"   ${Mapper_BUILD_QT})
option(Mapper_PACKAGE_ASSISTANT "Include Qt Assistant in the packages"                 ${Mapper_BUILD_QT})
mark_as_advanced(Mapper_BUILD_PACKAGE Mapper_PACKAGE_QT Mapper_PACKAGE_ASSISTANT Mapper_PACKAGE_PROJ)

unset(Mapper_PACKAGE_LIBRARIES CACHE) # Legacy


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
	set(CPACK_DEBIAN_PACKAGE_NAME  "openorienteering-mapper")
	set(MAPPER_RUNTIME_DESTINATION "/usr/bin")
	set(MAPPER_LIBRARY_DESTINATION "/usr/lib/${CPACK_DEBIAN_PACKAGE_NAME}")
	set(MAPPER_DATA_DESTINATION    "/usr/share/${CPACK_DEBIAN_PACKAGE_NAME}")
	set(MAPPER_ABOUT_DESTINATION   "/usr/share/doc/${CPACK_DEBIAN_PACKAGE_NAME}")
endif()

# Build definitons

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CMake28Quirks)

if(APPLE AND Mapper_MACOSX_VERSION)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=${Mapper_MACOSX_VERSION}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=${Mapper_MACOSX_VERSION}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mmacosx-version-min=${Mapper_MACOSX_VERSION}")
endif()

add_custom_target(Mapper_prerequisites)
set(Mapper_prerequisites_FOUND TRUE)

add_subdirectory("3rd-party/clipper")
add_dependencies(Mapper_prerequisites Clipper)
include_directories(${CLIPPER_INCLUDE_DIR})

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

if(Mapper_BUILD_QT AND Mapper_USE_QT5)
	add_subdirectory(3rd-party/qt5)
	add_dependencies(Mapper_prerequisites Qt5)
	list(INSERT CMAKE_PREFIX_PATH 0 "${QT5_DIR}")
	find_package(Qt5Core QUIET)
	if(NOT Qt5Core_FOUND)
		set(Mapper_prerequisites_FOUND FALSE)
		message(STATUS "Qt5 library will be built from source")
	endif()
elseif(Mapper_BUILD_QT)
	add_subdirectory(3rd-party/qt4)
	add_dependencies(Mapper_prerequisites Qt4)
	if(NOT QT_QMAKE_EXECUTABLE MATCHES NOTFOUND)
		find_package(Qt4 4.6 COMPONENTS QtCore QtGui QtNetwork QtXml)
	endif()
	if(NOT QT4_FOUND)
		set(Mapper_prerequisites_FOUND FALSE)
		message(STATUS "Qt4 library will be built from source")
	endif()
elseif(Mapper_USE_QT5)
	# may require -DCMAKE_PREFIX_PATH=/path-to-qt5-dir
	find_package(Qt5Core REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")
else()
	find_package(Qt4 4.6 REQUIRED QtCore QtGui QtNetwork QtXml)
endif()

if(Mapper_prerequisites_FOUND)

if(Mapper_USE_QT5)
	include(Qt5Quirks)
	foreach(qt5_module Qt5Core Qt5Widgets Qt5Xml Qt5Network Qt5PrintSupport)
		find_package(${qt5_module} REQUIRED)
		add_definitions(${${qt5_module}_DEFINITIONS})
		list(APPEND QT_INCLUDES ${${qt5_module}_INCLUDE_DIRS})
		list(APPEND QT_LIBRARIES ${${qt5_module}_LIBRARIES})
	endforeach()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")
else()
	include(${QT_USE_FILE})
	Quirks_Qt4_definitions()
	Quirks_Qt4_lib_dependencies()
endif()

if(CMAKE_CROSSCOMPILING)
	message(STATUS "Crosscompiling, host: ${CMAKE_HOST_SYSTEM_NAME}, target: ${CMAKE_SYSTEM_NAME}")

	if(Mapper_USE_QT5)
		qt5_find_host_program(Qt5Core_MOC_EXECUTABLE NAMES moc)
		qt5_find_host_program(Qt5Core_RCC_EXECUTABLE NAMES rcc)
	endif()
endif()

if(UNIX AND NOT APPLE)
#	set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib/${CPACK_DEBIAN_PACKAGE_NAME}/lib")
	set(CMAKE_INSTALL_RPATH "${MAPPER_LIBRARY_DESTINATION}/lib")
endif()

if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-long-long -Wno-deprecated -Wall -pedantic")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
	set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O2 -s")
	set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG")

	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
	set(CMAKE_C_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_RELEASE})
	set(CMAKE_C_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
else (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")
endif (CMAKE_COMPILER_IS_GNUCXX)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${PROJ_INCLUDE_DIR})

if(Mapper_DEVELOPMENT_BUILD)
	add_definitions(-DMAPPER_DEVELOPMENT_BUILD)
else()
	add_definitions(-DQT_NO_DEBUG -DQT_NO_DEBUG_OUTPUT -DQT_NO_WARNING_OUTPUT -DNDEBUG)
endif()

if(Mapper_DEVELOPMENT_BUILD OR NOT CMAKE_BUILD_TYPE MATCHES Release|MinSizeRel)
	set(Mapper_VERSION_DISPLAY "${CMAKE_BUILD_TYPE} ${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}")
else()
	set(Mapper_VERSION_DISPLAY "${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}")
endif()

# Subdirectories

add_subdirectory("examples")
add_subdirectory("symbol sets")
add_subdirectory("help")
add_subdirectory("translations")
add_subdirectory("3rd-party/qtsingleapplication")
add_subdirectory("libocad")
add_subdirectory("src")
if(Mapper_BUILD_PACKAGE)
	add_subdirectory("packaging")
endif()
if(CMAKE_CROSSCOMPILING)
	add_custom_target(TEST_WARNING ALL
	  COMMENT "Crosscompiling, skipping all tests")
	add_dependencies(TEST_WARNING Mapper)
elseif(Mapper_DEVELOPMENT_BUILD)
	enable_testing()
	add_subdirectory("test")
else()
	add_custom_target(TEST_WARNING ALL
	  COMMENT "Not a development build, skipping all tests")
	add_dependencies(TEST_WARNING Mapper)
endif()
add_subdirectory("docs/api")

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
