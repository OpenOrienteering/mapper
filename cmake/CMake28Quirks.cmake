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

