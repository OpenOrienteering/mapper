#
#    Copyright 2012, 2013 Kai Pastor
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

set(QT5_VERSION_DEFAULT 5.0.1)
set(QT5_VERSION ${QT5_VERSION_DEFAULT} CACHE STRING
  "Version number of the Qt5 library, minimum recommended value: 5.0.0")
mark_as_advanced(QT5_VERSION)

message(STATUS "Configuring Qt ${QT5_VERSION}")
 
if(QT5_VERSION STREQUAL 5.0.0)
	set(QT5_MD5 1a1b73f913a965f6b0cf02bb820c8a03)
	message(WARNING "Using Qt ${QT5_VERSION} which has been superseded by Qt ${QT5_VERSION_DEFAULT}.")
elseif(QT5_VERSION STREQUAL 5.0.1-rc1)
	set(QT5_MD5 e74765aa1620ff56d4642119078628c3)
	message(WARNING "Using Qt ${QT5_VERSION} which has been superseded by Qt ${QT5_VERSION_DEFAULT}.")
elseif(QT5_VERSION STREQUAL 5.0.1)
	set(QT5_MD5 8e8137c23c8ff078a560d3c62b4383e7)
elseif(QT5_VERSION STREQUAL 5.0.2)
	set(QT5_MD5 87cae8ae2f82f41ba027c2db0ab639b7)
elseif(QT5_VERSION STREQUAL 5.1.0-alpha)
	set(QT5_MD5 73d147a8965751850ff7d8b6a56d8366)
endif()

string(REGEX MATCH "5.." QT5_MAJOR_MINOR ${QT5_VERSION})
string(REGEX MATCH "5...." QT5_MAJOR_MINOR_REVISION ${QT5_VERSION})
if(QT5_VERSION MATCHES -)
	message(WARNING "Using a development release of Qt.")
	set(QT5_URL "http://download.qt-project.org/development_releases/qt/${QT5_MAJOR_MINOR}/${QT5_VERSION}/single/qt-everywhere-opensource-src-${QT5_VERSION}.tar.gz")
else()
	set(QT5_URL "http://download.qt-project.org/official_releases/qt/${QT5_MAJOR_MINOR}/${QT5_VERSION}/single/qt-everywhere-opensource-src-${QT5_VERSION}.tar.gz")
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

if(UNIX AND NOT APPLE)
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

if(${MAPPER_LIBRARY_DESTINATION} STREQUAL .)
	set(QT5_BINARY_DESTINATION "/binary")
elseif(NOT ${MAPPER_LIBRARY_DESTINATION} MATCHES "^/")
	set(QT5_BINARY_DESTINATION "/binary/${MAPPER_LIBRARY_DESTINATION}")
else()
	set(QT5_BINARY_DESTINATION "${MAPPER_LIBRARY_DESTINATION}")
endif()
set(QT5_PLUGINS_DESTINATION "${QT5_BINARY_DESTINATION}/plugins")
if(${MAPPER_DATA_DESTINATION} STREQUAL .)
	set(QT5_DATA_DESTINATION "/data")
elseif(NOT ${MAPPER_DATA_DESTINATION} MATCHES "^/")
	set(QT5_DATA_DESTINATION "/data/${MAPPER_DATA_DESTINATION}")
else()
	set(QT5_DATA_DESTINATION "${MAPPER_DATA_DESTINATION}")
endif()
# Cached only for export, not for reuse
set(QT5_DIR "${INSTALL_DIR}${QT5_BINARY_DESTINATION}" CACHE INTERNAL
  "The Qt5 target platform directory." FORCE)
set(QT_TRANSLATIONS_DIR "${INSTALL_DIR}/${QT5_DATA_DESTINATION}/translations" CACHE INTERNAL
  "The Qt5 translation directory." FORCE)
if(QT5_INSTALL_ROOT)
	set(QT5_CONFIG_EXTRA_OPTS "-R '${QT5_BINARY_DESTINATION}/lib' ${QT5_CONFIG_EXTRA_OPTS}")
	if(Mapper_DEVELOPMENT_BUILD)
		set(QT5_CONFIG_EXTRA_OPTS "-R '${INSTALL_DIR}${QT5_BINARY_DESTINATION}/lib' ${QT5_CONFIG_EXTRA_OPTS}")
	endif()
else()
	set(QT5_BINARY_DESTINATION "${INSTALL_DIR}${QT5_BINARY_DESTINATION}")
	set(QT5_PLUGINS_DESTINATION "${INSTALL_DIR}${QT5_PLUGINS_DESTINATION}")
	set(QT5_DATA_DESTINATION "${INSTALL_DIR}${QT5_DATA_DESTINATION}")
endif()
if(APPLE)
	set(QT5_CONFIG_EXTRA_OPTS "${QT5_CONFIG_EXTRA_OPTS} -no-framework")
endif()
if(UNIX AND NOT APPLE)
	set(QT5_CONFIG_EXTRA_OPTS "${QT5_CONFIG_EXTRA_OPTS} -qt-xcb")
endif()
if (CMAKE_CROSSCOMPILING)
	set(QT5_HOST_DIR "${INSTALL_DIR}/local" CACHE INTERNAL
	  "The Qt5 host platform directory." FORCE)
else()
	set(QT5_HOST_DIR ${QT5_DIR} CACHE INTERNAL
	  "The Qt5 host platform directory." FORCE)
endif()

configure_file(qt5-patchqt.in qt5-patchqt.new @ONLY)
configure_file(${QT5_CONFIG_SCRIPT}.in ${QT5_CONFIG_SCRIPT}.new @ONLY)
configure_file(qt5-make.in qt5-make @ONLY)
configure_file(qt.conf.in qt.conf @ONLY)
configure_file(qt.conf.qrc.in qt.conf.qrc @ONLY)

if(UNIX AND QT5_INSTALL_ROOT)
	# Wrappers which set LD_LIBRARY_PATH.
	configure_file(qt5-wrapper.in qcollectiongenerator @ONLY)
	configure_file(qt5-wrapper.in lupdate @ONLY)
	execute_process(
	  COMMAND chmod +x qcollectiongenerator lupdate
	  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" )
	set(QT_QCOLLECTIONGENERATOR_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/qcollectiongenerator"
	  CACHE PATH "The qcollectiongenerator wrapper.")
	set(QT_LUPDATE_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/lupdate"
	  CACHE PATH "The lupdate wrapper.")
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
# qcollectiongenerator (required) and lupdate (optional)

if(CMAKE_CROSSCOMPILING)
	set(QT5_EXTRA_HOST_DIR "${BINARY_DIR}/../${PROJECT_NAME}-build-host")
	if(NOT EXISTS "${QT5_EXTRA_HOST_DIR}/qtbase/Makefile")
		# Required for initial confclean-host
		file(MAKE_DIRECTORY "${QT5_EXTRA_HOST_DIR}/qtbase")
		file(WRITE "${QT5_EXTRA_HOST_DIR}/qtbase/Makefile" "confclean:")
	endif()
	configure_file(qt5-config-host.in qt5-config-host.new @ONLY)
	add_custom_command(OUTPUT qt5-config-host
	  COMMAND ${CMAKE_COMMAND} -E copy_if_different qt5-config-host.new qt5-config-host
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/qt5-config-host.new
	)
	ExternalProject_Add_Step(Qt5 confclean-host
	  COMMAND ${QT5_MAKE_COMMAND} -C "${QT5_EXTRA_HOST_DIR}/qtbase" confclean
	  DEPENDEES patch
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/qt5-config-host
	)
	ExternalProject_Add_Step(Qt5 configure-host
	  COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/qt5-config-host
	  DEPENDEES confclean-host
	  WORKING_DIRECTORY "${QT5_EXTRA_HOST_DIR}"
	)
	ExternalProject_Add_Step(Qt5 build-host
	  COMMAND ${QT5_MAKE_COMMAND} -C "${QT5_EXTRA_HOST_DIR}"
	  DEPENDEES configure-host
	)
	ExternalProject_Add_Step(Qt5 install-host
	  COMMAND ${QT5_MAKE_COMMAND} -C "${QT5_EXTRA_HOST_DIR}" install
	  DEPENDEES build-host
	  DEPENDERS install
	)
endif()

# Don't let Xcode re-root the install
set_target_properties(Qt5 PROPERTIES XCODE_ATTRIBUTE_INSTALL_ROOT "")

message(STATUS "Configuring Qt ${QT5_VERSION} - done")
 
