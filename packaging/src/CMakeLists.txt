#
#    Copyright 2013, 2014, 2018 Kai Pastor
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
 
message(STATUS "Configuring ${PROJECT_NAME} source packaging")

set(Mapper_Source_PREFIX
  "openorienteering-mapper-${Mapper_VERSION}")
	
set(Mapper_Source_FILE_NAME
  "openorienteering-mapper_${Mapper_VERSION}-src")
	
set(Mapper_Source_FORMAT tgz CACHE STRING
  "The archive format for source packages (see `git archive -l` for valid options)")

find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE)
	configure_file(Mapper_Source.cmake.in Mapper_Source.cmake @ONLY)
	add_custom_target(Mapper_Source
	  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/Mapper_Source.cmake"
	)
endif()
