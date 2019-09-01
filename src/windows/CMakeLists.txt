#
#    Copyright 2016-2017 Kai Pastor
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

find_package(Qt5Core REQUIRED)
find_package(Qt5Positioning)
set(CMAKE_AUTOMOC ON)
 
qt5_add_resources(MAPPER_WINDOWS_RESOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/windows.qrc"
  OPTIONS -no-compress
)

# Extra header to be shown in the IDE or to be translated
set(MAPPER_WINDOWS_HEADERS
  #
)
	
set(MAPPER_WINDOWS_SOURCES
  powershell_position_info_source.cpp
)
	
mapper_translations_sources(${MAPPER_WINDOWS_HEADERS} ${MAPPER_WINDOWS_SOURCES})

add_library(mapper-windows STATIC
  ${MAPPER_WINDOWS_RESOURCES}
  ${MAPPER_WINDOWS_HEADERS}
  ${MAPPER_WINDOWS_SOURCES}
)

target_compile_definitions(mapper-windows PRIVATE
  QT_NO_CAST_FROM_ASCII
  QT_NO_CAST_TO_ASCII
  QT_USE_QSTRINGBUILDER
)
target_include_directories(mapper-windows PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(mapper-windows PUBLIC Qt5::Core)
if(TARGET Qt5::Positioning)
	target_link_libraries(mapper-windows PUBLIC Qt5::Positioning)
	target_compile_definitions(mapper-windows INTERFACE MAPPER_WINDOWS_POSITIONING)
endif()
