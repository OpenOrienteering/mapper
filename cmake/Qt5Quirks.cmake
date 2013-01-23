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
 
macro(QT4_WRAP_CPP)
	qt5_wrap_cpp(${ARGV})
endmacro()

macro(QT4_ADD_RESOURCES)
	qt5_add_resources(${ARGV})
endmacro()

macro(QT4_ADD_TRANSLATION)
	qt5_add_translation(${ARGV})
endmacro()

if(CMAKE_CROSSCOMPILING)
# Cross-compilation: Qt 5.0.0's cmake modules point to the target tools,
# but we need the hosts tools.
	macro(QT5_FIND_HOST_PROGRAM VAR)
		# Get rid of non-cached Qt5 configuration variable
		unset(${VAR})
		if(NOT HOST_${VAR}_FOUND)
			# Search QT5_HOST_DIR first
			unset(${VAR} CACHE)
			find_program(${ARGV}
			  HINTS ${QT5_HOST_DIR}
			  PATH_SUFFIXES bin
			  NO_DEFAULT_PATH
			)
			if(${VAR} MATCHES NOTFOUND)
				message("${VAR}: not found in ${QT5_HOST_DIR}")
			else()
				set(HOST_${VAR}_FOUND TRUE CACHE INTERNAL "Found program in QT5_HOST_DIR")
			endif()
		endif()
		if (NOT HOST_${VAR}_FOUND)
			# This will search QT5_DIR since it is in CMAKE_PREFIX_PATH
			find_program(${ARGV})
			if(NOT ${VAR} MATCHES NOTFOUND)
				message("${VAR}: using alternative ${${VAR}}")
			endif()
		endif()
	endmacro()
else()
	macro(QT5_FIND_HOST_PROGRAM VAR)
		# This will search QT5_DIR since it is in CMAKE_PREFIX_PATH
		find_program(${ARGV})
	endmacro()
endif()
