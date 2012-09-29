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


# This file contains workaround for a number of issues with particular
# revisions of CMake 2.8
#
cmake_minimum_required(VERSION 2.8)



# CMAKE_CURRENT_LIST_DIR is not available before cmake 2.8.3
# 
macro(QUIRKS_CMAKE_CURRENT_LIST_DIR)
	if(CMAKE_VERSION VERSION_LESS "2.8.3")
		get_filename_component(CMAKE_CURRENT_LIST_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
	endif()
endmacro(QUIRKS_CMAKE_CURRENT_LIST_DIR)



# When linking to static Qt libs, QT_DEFINITIONS must not contain -DQT_DLL.
# Otherwise linking will fail. CMake 2.8.0 doesn't handle this properly.
# Problem also indicated here:
#  http://lists.gnu.org/archive/html/mingw-cross-env-list/2010-11/msg00069.html
#
# Use this after FindQt4, but before UseQt4.
#
macro(QUIRKS_QT4_DEFINITIONS)
	if(NOT ${QT4_FOUND})
		message(FATAL_ERROR "You must call FIND_PACKAGE(Qt4 ...) before QUIRKS_QT4_DEFINITIONS()") 
	endif()
	if(QT_DEFINITIONS MATCHES -DQT_DLL)
		find_file(_qtcore_prl "QtCore.prl" "${QT_LIBRARY_DIR}")
		if(_qtcore_prl)
			file(STRINGS "${_qtcore_prl}" _prl_config REGEX "QMAKE_PRL_CONFIG")
			if("${_prl_config}" MATCHES " static[^l]")
				list(REMOVE_ITEM QT_DEFINITIONS -DQT_DLL)
				remove_definitions(-DQT_DLL)
			endif()
		endif()
	endif()
endmacro(QUIRKS_QT4_DEFINITIONS)



# When linking to Qt libs, it may be neccessary to link additional libraries.
# The only place to find the required libs seems to be the QMAKE_PRL_LIBS 
# field of the .prl files which accompany the .a library files.
# For another implementation, see Daniel Stonier's message here:
#  http://lists.gnu.org/archive/html/mingw-cross-env-list/2010-11/msg00069.html
#
# Use this after INCLUDE(${QT_USE_FILE})  [INCLUDE(UseQt4)]
#
macro(QUIRKS_QT4_LIB_DEPENDENCIES)
	if(NOT QT_LIBRARIES)
		message(FATAL_ERROR "You must call INCLUDE(\${QT_USE_MODULE}) before QUIRKS_QT4_LIB_DEPENDENCIES()") 
	endif()
	foreach(QT_MODULE ${QT_MODULES}) # from FindQt4
		string(TOUPPER ${QT_MODULE} _upper_qt_module)
		if( QT_${_upper_qt_module}_FOUND AND
		   (QT_USE_${_upper_qt_module} OR QT_USE_${_upper_qt_module}_DEPENDS) )

			set(_module_prl NOTFOUND)
	 		find_file(_module_prl 
			          NAMES "${QT_MODULE}.prl"S "lib${QT_MODULE}.prl"
			          PATHS "${QT_LIBRARY_DIR}")
		 	if(_module_prl)
 				file(STRINGS "${_module_prl}" _prl_libs REGEX "QMAKE_PRL_LIBS")
	 			string(REGEX REPLACE "QMAKE_PRL_LIBS[ \t]*=" "" _prl_libs "${_prl_libs}")
		 		string(REGEX MATCHALL "[^ \t\n]+" _linker_flags "${_prl_libs}")
	 			if(_linker_flags MATCHES "^-l")
    	 			list(REMOVE_ITEM QT_LIBRARIES ${_linker_flags})
	    	 		list(APPEND QT_LIBRARIES ${_linker_flags})
		     	endif()
 			endif(_module_prl)
		endif()
	endforeach(QT_MODULE)
	foreach(QT_MODULE ${QT_MODULES})
		list(REMOVE_ITEM QT_LIBRARIES "-l${QT_MODULE}")
	endforeach(QT_MODULE)
endmacro(QUIRKS_QT4_LIB_DEPENDENCIES)



# If the Windows resource compiler is windres, some revision of CMake 2.8
# specify incorrect parameters.
# For CMake<=2.8.3, cf. http://public.kitware.com/Bug/view.php?id=4068
# For CMake<=2.8.7, cf. http://public.kitware.com/Bug/view.php?id=12480
#
macro(QUIRKS_CMAKE_RC_COMPILER)
	if(NOT CMAKE_RC_COMPILER)
		set(CMAKE_RC_COMPILER windres)
	endif()
	if(CMAKE_RC_COMPILER MATCHES "windres")
		if(NOT CMAKE_RC_OUTPUT_EXTENSION MATCHES ".obj")
			set(CMAKE_RC_OUTPUT_EXTENSION .obj CACHE INTERNAL "" FORCE)
		endif()
		if(CMAKE_RC_COMPILE_OBJECT MATCHES "/fo")
			set(CMAKE_RC_COMPILE_OBJECT
			    "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -o <OBJECT> -i <SOURCE>")
		endif()
	endif(CMAKE_RC_COMPILER MATCHES "windres")
endmacro(QUIRKS_CMAKE_RC_COMPILER)

