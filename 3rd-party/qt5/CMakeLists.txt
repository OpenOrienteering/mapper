#
#    Copyright 2012, 2013, 2014 Kai Pastor
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


project(Qt5)

cmake_minimum_required(VERSION 2.8.3)

include(ExternalProject)

# Configuration options

set(QT5_VERSION_DEFAULT 5.3.2)
set(QT5_VERSION ${QT5_VERSION_DEFAULT} CACHE STRING
  "Version number of the Qt5 library, recommended value: ${QT5_VERSION_DEFAULT}")
mark_as_advanced(QT5_VERSION)


if(NOT MAPPER_LIBRARY_DESTINATION AND CMAKE_BUILD_TYPE MATCHES "Debug")
	# stand-alone debug build of Qt5
	set(QT5_DEBUG_DEFAULT On)
else()
	set(QT5_DEBUG_DEFAULT Off)
endif()
set(QT5_DEBUG ${QT5_DEBUG_DEFAULT} CACHE BOOL
  "Enable debug build of Qt"
)
mark_as_advanced(QT5_DEBUG)

message(STATUS "Configuring Qt ${QT5_VERSION}")
 
if (NOT ${QT5_VERSION} STREQUAL ${QT5_VERSION_DEFAULT})
	message(WARNING
	  "The Qt5 library version is different from the current recommended version "
	  "(${QT5_VERSION} vs. ${QT5_VERSION_DEFAULT}).")
endif()

set(QT5_MD5SUMS
  # Schema: VERSION:MD5[:DOWNLOAD_URL]
  5.2.1:a78408c887c04c34ce615da690e0b4c8
  5.3.1:a355749e4f200a6392ecad151d1d0cdc
  5.3.2:febb001129927a70174467ecb508a682
)
foreach(line ${QT5_MD5SUMS})
	if(${line} MATCHES "^${QT5_VERSION}:")
		string(REPLACE "${QT5_VERSION}:" "" QT5_MD5 ${line})
		break()
	endif()
endforeach()
if(NOT QT5_MD5)
	message(FATAL_ERROR
	  "Unknown MD5 sum for Qt5 library ${QT5_VERSION}. "
	  "Edit ${PROJECT_SOURCE_DIR}/CMakeLists.txt, "
	  "or specify the correct QT5_MD5 value at the command line.")
endif()

string(REGEX MATCH "5.." QT5_MAJOR_MINOR ${QT5_VERSION})
string(REGEX MATCH "5...." QT5_MAJOR_MINOR_REVISION ${QT5_VERSION})
set(QT5_MAJOR_MINOR_REVISION ${QT5_MAJOR_MINOR_REVISION} CACHE STRING
  "The Qt version number without additional release markers" FORCE) # Export
if(QT5_MD5 MATCHES :)
	message(WARNING "Not using an official release of Qt.")
	string(REGEX REPLACE "^[0-9a-fA-F]*:" "" QT5_URL "${QT5_MD5}")
	string(REGEX REPLACE ":.*"      "" QT5_MD5 "${QT5_MD5}")
else()
	set(QT5_URL "http://download.qt-project.org/archive/qt/${QT5_MAJOR_MINOR}/${QT5_VERSION}/single/qt-everywhere-opensource-src-${QT5_VERSION}.tar.gz")
endif()

find_program(SH_PROGRAM sh 
  PATHS 
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS-1.0_is1;Inno Setup: App Path]/bin"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MinGW;InstallLocation]/bin"
  C:
  C:/MinGW
  PATH_SUFFIXES msys/1.0/bin)
mark_as_advanced(SH_PROGRAM)

if("${CMAKE_GENERATOR}" MATCHES "Makefiles" OR
   "${CMAKE_EXTRA_GENERATOR}" MATCHES "Makefiles")
	# use magic MAKE macro
	set(QT5_MAKE_COMMAND "\$(MAKE)")
else()
	# require make
	set(QT5_MAKE_COMMAND "${SH_PROGRAM}" -l -e "${CMAKE_CURRENT_BINARY_DIR}/qt5-make")
endif()

if(MINGW AND NOT CMAKE_CROSSCOMPILING)
	set(QT5_CONFIG_SCRIPT qt5-config.cmd)
	set(QT5_CONFIG_COMMAND "$ENV{ComSpec}" /C ${CMAKE_CURRENT_BINARY_DIR}/${QT5_CONFIG_SCRIPT})
	set(QT5_SOURCE_CONFIG 
	  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/src"
	  BUILD_IN_SOURCE 1
	)
else()
	set(QT5_CONFIG_SCRIPT qt5-config)
	set(QT5_CONFIG_COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/${QT5_CONFIG_SCRIPT})
	set(QT5_SOURCE_CONFIG )
	if(MINGW AND CMAKE_CROSSCOMPILING AND NOT QT5_PLATFORM)
		string(REGEX REPLACE gcc\$ "" QT5_CROSS_COMPILE ${CMAKE_C_COMPILER})
		set(QT5_PLATFORM "-xplatform win32-g++ -device-option CROSS_COMPILE='${QT5_CROSS_COMPILE}'")
	endif()
endif()

# Build definition

# Don't touch the configured qt5-patchqt unless the content really changes.
add_custom_command(OUTPUT qt5-patchqt
  COMMAND ${CMAKE_COMMAND} -E copy_if_different qt5-patchqt.new qt5-patchqt
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/qt5-patchqt.new
)

# Don't touch the configured qt5-config unless the content really changes.
add_custom_command(OUTPUT ${QT5_CONFIG_SCRIPT}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT5_CONFIG_SCRIPT}.new ${QT5_CONFIG_SCRIPT}
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${QT5_CONFIG_SCRIPT}.new
)

if(UNIX AND NOT APPLE AND MAPPER_LIBRARY_DESTINATION)
	set(QT5_INSTALL_ROOT "INSTALL_ROOT=<INSTALL_DIR>")
endif()
ExternalProject_Add(Qt5
  DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/download
  URL ${QT5_URL}
  URL_MD5 ${QT5_MD5}
  PATCH_COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/qt5-patchqt
  CONFIGURE_COMMAND ${QT5_CONFIG_COMMAND}
  BUILD_COMMAND ${QT5_MAKE_COMMAND}
  INSTALL_COMMAND ${QT5_MAKE_COMMAND} install ${QT5_INSTALL_ROOT}
  ${QT5_SOURCE_CONFIG}
)
ExternalProject_Get_Property(Qt5 SOURCE_DIR)
ExternalProject_Get_Property(Qt5 BINARY_DIR)
ExternalProject_Get_Property(Qt5 INSTALL_DIR)
set_target_properties(Qt5 PROPERTIES INSTALL_DIR "${INSTALL_DIR}")

# Trigger patch step when qt5-patchqt has changed.
ExternalProject_Add_Step(Qt5 patch-dependencies
  COMMAND ${CMAKE_COMMAND} -E echo_append "" # Do nothing
  DEPENDEES download update
  DEPENDERS patch
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/qt5-patchqt
)

# Trigger confclean and configure steps when qt5-config has changed.
if(NOT "${SOURCE_DIR}" STREQUAL "${BINARY_DIR}")
ExternalProject_Add_Step(Qt5 confclean
  COMMAND ${CMAKE_COMMAND} -E remove_directory "${BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_DIR}"
  DEPENDEES patch
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${QT5_CONFIG_SCRIPT}
  WORKING_DIRECTORY "${SOURCE_DIR}"
)
ExternalProject_Add_Step(Qt5 configure-dependencies
  COMMAND ${CMAKE_COMMAND} -E echo_append "" # Do nothing
  DEPENDEES confclean
  DEPENDERS configure
  WORKING_DIRECTORY "${BINARY_DIR}"
)
else()
# In-source-build: re-trigger extract; download will be avoided by MD5 sum.
ExternalProject_Add_Step(Qt5 confclean
  COMMAND ${CMAKE_COMMAND} -E echo_append "" # Do nothing
  DEPENDERS download
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${QT5_CONFIG_SCRIPT}
  WORKING_DIRECTORY "${SOURCE_DIR}"
)
endif()

# Build script configuration

# Debug or release build
if (QT5_DEBUG)
	set(QT5_CONFIG_EXTRA_OPTS "-debug -qml-debug ${QT5_CONFIG_EXTRA_OPTS}")
else()
	set(QT5_CONFIG_EXTRA_OPTS "-release -no-qml-debug ${QT5_CONFIG_EXTRA_OPTS}")
endif()

# Paths
if(NOT MAPPER_LIBRARY_DESTINATION)
	# stand-alone build of Qt5
	set(QT5_BINARY_DESTINATION  "")
	set(QT5_PLUGINS_DESTINATION "/plugins")
elseif(MAPPER_LIBRARY_DESTINATION MATCHES "^/")
	# absolute path
	set(QT5_BINARY_DESTINATION  "${MAPPER_LIBRARY_DESTINATION}")
	set(QT5_PLUGINS_DESTINATION "${MAPPER_LIBRARY_DESTINATION}/plugins")
elseif(MAPPER_LIBRARY_DESTINATION STREQUAL ".")
	# .
	set(QT5_BINARY_DESTINATION  "/binary")
	set(QT5_PLUGINS_DESTINATION "/binary/plugins")
else()
	# relative path other than .
	set(QT5_BINARY_DESTINATION  "/binary/${MAPPER_LIBRARY_DESTINATION}")
	set(QT5_PLUGINS_DESTINATION "/binary/${MAPPER_LIBRARY_DESTINATION}/plugins")
endif()
if(NOT MAPPER_DATA_DESTINATION)
	# stand-alone build of Qt5
	set(QT5_DATA_DESTINATION "")
elseif(MAPPER_DATA_DESTINATION MATCHES "^/")
	# absolute path
	set(QT5_DATA_DESTINATION "${MAPPER_DATA_DESTINATION}")
elseif(MAPPER_DATA_DESTINATION STREQUAL .)
	# .
	set(QT5_DATA_DESTINATION "/data")
else()
	# relative path other than .
	set(QT5_DATA_DESTINATION "/data/${MAPPER_DATA_DESTINATION}")
endif()

# Cached only for export, not for reuse
set(QT5_DIR "${INSTALL_DIR}${QT5_BINARY_DESTINATION}" CACHE INTERNAL
  "The Qt5 target platform directory." FORCE)
set(QT_TRANSLATIONS_DIR "${INSTALL_DIR}${QT5_DATA_DESTINATION}/translations" CACHE INTERNAL
  "The Qt5 translation directory." FORCE)

if(NOT QT5_INSTALL_ROOT)
	set(QT5_BINARY_DESTINATION "${INSTALL_DIR}${QT5_BINARY_DESTINATION}")
	set(QT5_PLUGINS_DESTINATION "${INSTALL_DIR}${QT5_PLUGINS_DESTINATION}")
	set(QT5_DATA_DESTINATION "${INSTALL_DIR}${QT5_DATA_DESTINATION}")
endif()

# Extra configuration options
if(QT5_INSTALL_ROOT AND NOT "${QT5_PLATFORM}" MATCHES "android")
	set(QT5_CONFIG_EXTRA_OPTS "-R '${QT5_BINARY_DESTINATION}/lib' ${QT5_CONFIG_EXTRA_OPTS}")
	if(Mapper_DEVELOPMENT_BUILD)
		set(QT5_CONFIG_EXTRA_OPTS "-R '${INSTALL_DIR}${QT5_BINARY_DESTINATION}/lib' ${QT5_CONFIG_EXTRA_OPTS}")
	endif()
endif()
if(APPLE)
	set(QT5_CONFIG_EXTRA_OPTS "${QT5_CONFIG_EXTRA_OPTS} -no-framework")
endif()
if(UNIX AND NOT APPLE AND NOT "${QT5_PLATFORM}" MATCHES "android")
	set(QT5_CONFIG_ENVIRONMENT "LDFLAGS=-Wl,--enable-new-dtags")
	set(QT5_CONFIG_EXTRA_OPTS "${QT5_CONFIG_EXTRA_OPTS} -qt-xcb")
endif()
if(${QT5_VERSION} MATCHES "5.[0-2]")
	set(QT5_CONFIG_EXTRA_OPTS "${QT5_CONFIG_EXTRA_OPTS} -no-javascript-jit")
endif()
if("${QT5_PLATFORM}" MATCHES "android")
	# cf. https://bugreports.qt-project.org/browse/QTBUG-38452
	set(QT5_CONFIG_EXTRA_OPTS "${QT5_CONFIG_EXTRA_OPTS} -nomake tools")
else()
	set(QT5_CONFIG_EXTRA_OPTS "${QT5_CONFIG_EXTRA_OPTS} -make tools")
endif()

configure_file(qt5-patchqt.in qt5-patchqt.new @ONLY)
configure_file(${QT5_CONFIG_SCRIPT}.in ${QT5_CONFIG_SCRIPT}.new @ONLY)
configure_file(qt5-make.in qt5-make @ONLY)
configure_file(qt.conf.in qt.conf @ONLY)
configure_file(qt.conf.qrc.in qt.conf.qrc @ONLY)

if(UNIX AND QT5_INSTALL_ROOT AND NOT "${QT5_PLATFORM}" MATCHES "android")
	file(WRITE "${INSTALL_DIR}${QT5_BINARY_DESTINATION}/bin/qt.conf"
	  "[Paths]\n"
	  "Plugins=${INSTALL_DIR}${QT5_PLUGINS_DESTINATION}\n"
	)
	set_target_properties(Qt5 PROPERTIES
	  PLUGINS_DIR "${INSTALL_DIR}${QT5_PLUGINS_DESTINATION}"
	)
endif()
function(WRITE_QT_CONF)
	get_target_property(qt5_plugins_dir Qt5 PLUGINS_DIR)
	if(NOT "${qt5_plugins_dir}" MATCHES NOTFOUND)
		file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
		  "[Paths]\n"
		  "Plugins=${qt5_plugins_dir}\n"
		)
	endif()
endfunction()

# Cross-compiling requires an extra host build in order to get 
# qcollectiongenerator.
# In addition, Linux may have trouble with loading the correct platform plugin
# version when building a newer Qt version than installed.

if((UNIX AND NOT APPLE) OR (CMAKE_CROSSCOMPILING AND NOT "${QT5_PLATFORM}" MATCHES "android"))
	set(QT5_EXTRA_HOST_DIR "${BINARY_DIR}/../Qt5-build-host")
	
	configure_file(qt5-config-host.in qt5-config-host.new @ONLY)
	if(NOT ${QT5_VERSION} MATCHES "5.[0-2]")
		execute_process(
		  COMMAND sed -i -e /-no-javascript-jit/d qt5-config-host.new
		  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" )
	endif()
	
	add_custom_command(OUTPUT qt5-config-host
	  COMMAND ${CMAKE_COMMAND} -E copy_if_different qt5-config-host.new qt5-config-host
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/qt5-config-host.new
	)
	ExternalProject_Add_Step(Qt5 confclean-host
	  COMMAND ${CMAKE_COMMAND} -E make_directory "${QT5_EXTRA_HOST_DIR}"
	  COMMAND ${CMAKE_COMMAND} -E remove_directory "${QT5_EXTRA_HOST_DIR}"
	  COMMAND ${CMAKE_COMMAND} -E make_directory "${QT5_EXTRA_HOST_DIR}"
	  DEPENDEES patch
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/qt5-config-host
	)
	ExternalProject_Add_Step(Qt5 configure-host
	  COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/qt5-config-host
	  DEPENDEES confclean-host
	  WORKING_DIRECTORY "${QT5_EXTRA_HOST_DIR}"
	)
	ExternalProject_Add_Step(Qt5 build-host
	  COMMAND ${QT5_MAKE_COMMAND} -C "${QT5_EXTRA_HOST_DIR}" module-qtdeclarative
	  COMMAND ${QT5_MAKE_COMMAND} -C "${QT5_EXTRA_HOST_DIR}" module-qttools-qmake_all
	  COMMAND ${QT5_MAKE_COMMAND} -C "${QT5_EXTRA_HOST_DIR}/qttools/src/assistant" sub-qcollectiongenerator
	  DEPENDEES configure-host
	)
	ExternalProject_Add_Step(Qt5 install-host
	  # not needed: COMMAND ${QT5_MAKE_COMMAND} -C "${QT5_EXTRA_HOST_DIR}/qttools/src/assistant" sub-qcollectiongenerator-install_subtargets
	  DEPENDEES build-host
	  DEPENDERS install
	)
	
	if(NOT TARGET Qt5::qcollectiongenerator)
		add_executable(Qt5::qcollectiongenerator IMPORTED GLOBAL)
		set(imported_location ${QT5_EXTRA_HOST_DIR}/qttools/bin/qcollectiongenerator)
		set_target_properties(Qt5::qcollectiongenerator PROPERTIES
			IMPORTED_LOCATION ${imported_location}
		)
		add_dependencies(Qt5::qcollectiongenerator Qt5)
	endif()
elseif(CMAKE_CROSSCOMPILING AND "${QT5_PLATFORM}" MATCHES "android")
	execute_process(
	  COMMAND sed -i -e "/-plugindir /d;/-datadir /d" ${QT5_CONFIG_SCRIPT}.new
	  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" )
endif()

# Don't let Xcode re-root the install
set_target_properties(Qt5 PROPERTIES XCODE_ATTRIBUTE_INSTALL_ROOT "")

message(STATUS "Configuring Qt ${QT5_VERSION} - done")
