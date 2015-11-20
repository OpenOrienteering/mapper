#
#    Copyright 2012 Kai Pastor
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


project(Qt4)

cmake_minimum_required(VERSION 2.8)
 
# Configuration options

if(NOT QT4_VERSION)
	set(QT4_VERSION 4.8.1 CACHE STRING
	  "Version number of the Qt library, minumum recommended value: 4.8.1")
endif(NOT QT4_VERSION)
mark_as_advanced(QT4_VERSION)

if(NOT QT4_MD5)
	set(QT4_MD5 7960ba8e18ca31f0c6e4895a312f92ff CACHE STRING
	  "MD5 checksum of the Qt library TGZ archive")
endif(NOT QT4_MD5)
mark_as_advanced(QT4_MD5)

if("${CMAKE_GENERATOR}" MATCHES "Makefiles" OR 
   "${CMAKE_EXTRA_GENERATOR}" MATCHES "Makefiles")
	# use magic MAKE macro
	set(QT4_MAKE_COMMAND "\$(MAKE)" CACHE STRING
	  "The 'make' command for building the Qt library")
else()
	# require make
	set(QT4_MAKE_COMMAND "${SH_PROGRAM}" -l -e "${CMAKE_CURRENT_BINARY_DIR}/qt4-make" make CACHE STRING
	  "The 'make' command for building the Qt library")
endif()
mark_as_advanced(QT4_MAKE_COMMAND)

find_program(SH_PROGRAM sh 
  PATHS 
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSYS-1.0_is1;Inno Setup: App Path]/bin"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MinGW;InstallLocation]/bin"
  C:
  C:/MinGW
  PATH_SUFFIXES msys/1.0/bin)
mark_as_advanced(SH_PROGRAM)

# Build definition

if(CMAKE_VERSION VERSION_GREATER 2.8.1)
	set(_ep_qt4_options URL_MD5 ${QT4_MD5})
else()
	set(_ep_qt4_options)
endif()

include(ExternalProject)
ExternalProject_Add(
  Qt4
  DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/download
  URL "http://releases.qt-project.org/qt4/source/qt-everywhere-opensource-src-${QT4_VERSION}.tar.gz"
  ${_ep_qt4_options}
  UPDATE_COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/qt4-syncqt
  PATCH_COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/qt4-patchqt
  CONFIGURE_COMMAND "${SH_PROGRAM}" -l -e ${CMAKE_CURRENT_BINARY_DIR}/qt4-config
  BUILD_COMMAND ${QT4_MAKE_COMMAND}
  INSTALL_COMMAND ${QT4_MAKE_COMMAND} install
)
ExternalProject_Get_Property(Qt4 SOURCE_DIR)
ExternalProject_Get_Property(Qt4 BINARY_DIR)
ExternalProject_Get_Property(Qt4 INSTALL_DIR)

configure_file(qt4-syncqt.in ${CMAKE_CURRENT_BINARY_DIR}/qt4-syncqt)
configure_file(qt4-patchqt.in ${CMAKE_CURRENT_BINARY_DIR}/qt4-patchqt)
configure_file(qt4-config.in ${CMAKE_CURRENT_BINARY_DIR}/qt4-config)
configure_file(qt4-make.in ${CMAKE_CURRENT_BINARY_DIR}/qt4-make)
if(QT_PLATFORM)
	# cross compilation
	configure_file(qmake.conf.in ${CMAKE_CURRENT_BINARY_DIR}/qmake.conf.tmp)
	configure_file(qplatformdefs.h.in ${CMAKE_CURRENT_BINARY_DIR}/qplatformdefs.h.tmp)
endif(QT_PLATFORM)

# Exported configuration

# qmake will be created early in the Qt the build.
# QT_QMAKE_EXECUTABLE will be used by find_package(Qt4 ...).

unset(QT_QMAKE_EXECUTABLE CACHE)
find_program(QT_QMAKE_EXECUTABLE NAMES qmake
  PATHS "${BINARY_DIR}/bin"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
mark_as_advanced(QT_QMAKE_EXECUTABLE)

