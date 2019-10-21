#
#    Copyright 2016 Kai Pastor
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


if(NOT COMMAND check_cxx_compiler_flag)
	include(CheckCXXCompilerFlag)
endif()

function(enable_sanitize)
	foreach(option ${ARGV})
		if(CMAKE_CROSSCOMPILING)
			break() # out of macro
		endif()
		if(option STREQUAL "NO_RECOVER")
			set(full_option "-fno-sanitize-recover=all -fno-omit-frame-pointer")
		else()
			set(full_option "-fsanitize=${option}")
		endif()
		if(NOT CMAKE_CXX_FLAGS MATCHES "${full_option}")
			set(CMAKE_REQUIRED_LIBRARIES "${full_option}")
			string(MAKE_C_IDENTIFIER ${option} option_id)
			check_cxx_compiler_flag("${full_option}" SANITIZE_${option_id})
			if(SANITIZE_${option_id})
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${full_option}" PARENT_SCOPE)
				set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${full_option}" PARENT_SCOPE)
			endif()
		endif()
	endforeach()
endfunction()

