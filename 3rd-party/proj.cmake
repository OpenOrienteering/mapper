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
 
set(PROJ_LIBRARY "proj")

if(NOT CMAKE_CURRENT_LIST_DIR)
	# CMAKE_CURRENT_LIST_DIR is not available before cmake 2.8.3
	get_filename_component(CMAKE_CURRENT_LIST_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
endif()

if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
	if(NOT PROJ_VERSION)
		set(PROJ_VERSION 4.8.0 CACHE STRING
		"Version number of the PROJ.4 library, minumum recommended value: 4.8.0"
		FORCE)
	endif(NOT PROJ_VERSION)
	if(NOT PROJ_MD5)
		set(PROJ_MD5 d815838c92a29179298c126effbb1537 CACHE STRING
		"MD5 checksum of the PROJ.4 library TGZ archive"
		FORCE)
	endif(NOT PROJ_MD5)
	if(NOT PROJ_MAKEFLAGS)
		set(PROJ_MAKEFLAGS -j3 CACHE STRING
		"Flags to pass to make for the PROJ.4 library build"
		FORCE)
	endif(NOT PROJ_MAKEFLAGS)
	
	add_library(${PROJ_LIBRARY} SHARED IMPORTED)
	set(PROJ_DIR "${CMAKE_CURRENT_LIST_DIR}/proj/build")
	set(PROJ_INCLUDES "${PROJ_DIR}/include")
	set(PROJ_LIBRARIES "${PROJ_DIR}/lib")
	set_target_properties(${PROJ_LIBRARY} PROPERTIES
	  IMPORTED_IMPLIB ${PROJ_DIR}/lib/libproj.dll.a
	  IMPORTED_LOCATION ${PROJ_DIR}/bin
	)
	add_custom_target(COPY_PROJ_DLL
	  COMMENT Copying libproj-0.dll to executable directory
	  COMMAND ${CMAKE_COMMAND} -E copy_if_different
	  "${PROJ_DIR}/bin/libproj-0.dll"
	  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libproj-0.dll"
	)
	# add_dependencies requires CMake >= 2.8.4,
	# cf. http://public.kitware.com/Bug/view.php?id=10395
	#add_dependencies(${PROJ_LIBRARY} COPY_PROJ_DLL)
	set_property(TARGET ${PROJ_LIBRARY} PROPERTY DEPENDS COPY_PROJ_DLL)

	SET(PROJ_MSYS_BASH "C:/MinGW/msys/1.0/bin/bash.exe")
	if(EXISTS "${PROJ_MSYS_BASH}")
		include(ExternalProject)
		ExternalProject_Add(
		  proj_external
		  URL http://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz
		  URL_MD5 ${PROJ_MD5}
		  TIMEOUT 30
		  SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/proj"
		  BUILD_IN_SOURCE 1
		  CONFIGURE_COMMAND "${PROJ_MSYS_BASH}" -c "pushd . && . /etc/profile && popd && ./configure --without-mutex --prefix=\"$PWD\"/build"
		  BUILD_COMMAND "${PROJ_MSYS_BASH}" -c "pushd . && . /etc/profile && popd && make ${PROJ_MAKEFLAGS}"
		  INSTALL_COMMAND "${PROJ_MSYS_BASH}" -c "pushd . && . /etc/profile && popd && make ${PROJ_MAKEFLAGS} install"
		)
		add_dependencies(COPY_PROJ_DLL proj_external )
	endif(EXISTS "${PROJ_MSYS_BASH}")

	list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${PROJ_DIR}/bin/libproj-0.dll")
endif()
