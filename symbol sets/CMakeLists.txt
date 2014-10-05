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
 
set(Mapper_SYMBOL_SETS
  4000/ISSOM_4000.omap
  5000/ISSOM_5000.omap
  10000/ISOM_10000.omap
  15000/ISOM_15000.omap
)

set(_symbol_sets)
foreach(_symbol_set ${Mapper_SYMBOL_SETS})
	get_filename_component(_dir  ${_symbol_set} PATH)
	install(FILES ${_symbol_set} DESTINATION "${MAPPER_DATA_DESTINATION}/symbol sets/${_dir}")

	# Files in the build tree may be overwritten or deleted.
	# Rename the symbol sets in order to give a minimal warning.
	get_filename_component(_name ${_symbol_set} NAME)
	set(_copy_name ${_dir}/COPY_OF_${_name})
	add_custom_command(OUTPUT ${_copy_name}
	  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${_symbol_set} ${CMAKE_CURRENT_BINARY_DIR}/${_copy_name}
	  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_symbol_set}
	  VERBATIM)
	list(APPEND _symbol_sets "${CMAKE_CURRENT_BINARY_DIR}/${_copy_name}")
endforeach()

# Copy symbol sets to the build tree
add_custom_target(Mapper_symbol_sets
  ALL
  DEPENDS ${_symbol_sets})

