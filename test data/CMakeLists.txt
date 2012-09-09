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
 
set(Mapper_TEST_DATA
  test_map.omap
)

set(_test_data)
foreach(_test_file ${Mapper_TEST_DATA})
	# Files in the build tree may be overwritten or deleted.
	# Rename the test files in order to give a minimal warning.
	get_filename_component(_name ${_test_file} NAME)
	set(_copy_name COPY_OF_${_name})
	add_custom_command(OUTPUT ${_copy_name}
	  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${_test_file} ${CMAKE_CURRENT_BINARY_DIR}/${_copy_name}
	  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_test_file}
	  VERBATIM)
	list(APPEND _test_data "${CMAKE_CURRENT_BINARY_DIR}/${_copy_name}")
endforeach()

# Copy test files to the build tree
add_custom_target(Mapper_test_data
  ALL
  DEPENDS ${_test_data})

