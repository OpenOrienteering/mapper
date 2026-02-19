#
#    Copyright 2016-2020, 2026 Kai Pastor
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

find_package(GDAL MODULE REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
 
# Extra header to be shown in the IDE or to be translated
set(MAPPER_GDAL_HEADERS
  ogr_file_format_p.h
)
	
set(MAPPER_GDAL_SOURCES
  gdal_file.cpp
  gdal_image_reader.cpp
  gdal_manager.cpp
  gdal_settings_page.cpp
  gdal_template.cpp
  kmz_groundoverlay_export.cpp
  ogr_file_format.cpp
  ogr_template.cpp
  mapper-osmconf.ini
)
	
mapper_translations_sources(${MAPPER_GDAL_HEADERS} ${MAPPER_GDAL_SOURCES})

add_library(mapper-gdal STATIC ${MAPPER_GDAL_HEADERS} ${MAPPER_GDAL_SOURCES} "${PROJECT_BINARY_DIR}/gdal/mapper-osmconf.ini")

target_compile_definitions(mapper-gdal PRIVATE
  QT_NO_CAST_FROM_ASCII
  QT_NO_CAST_TO_ASCII
  QT_USE_QSTRINGBUILDER
)

target_compile_definitions(mapper-gdal INTERFACE MAPPER_USE_GDAL)

target_include_directories(mapper-gdal SYSTEM PRIVATE ${GDAL_INCLUDE_DIRS})
target_include_directories(mapper-gdal PRIVATE "${PROJECT_SOURCE_DIR}/src")

target_link_libraries(mapper-gdal ${GDAL_LIBRARIES} Qt5::Core Qt5::Gui Qt5::Widgets Mapper_Common)

set_target_properties(mapper-gdal PROPERTIES PREFIX "")

install(FILES mapper-osmconf.ini DESTINATION "${MAPPER_DATA_DESTINATION}/gdal")

# Let mapper-osmconf.ini be found via "data:/gdal" during development.
add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/gdal/mapper-osmconf.ini"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "gdal"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/mapper-osmconf.ini" "gdal/mapper-osmconf.ini"
  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
)


# Utility to dump GDAL format information

add_executable(mapper-gdal-info mapper_gdal_info.cpp)
target_include_directories(mapper-gdal-info SYSTEM PRIVATE ${GDAL_INCLUDE_DIRS})
target_link_libraries(mapper-gdal-info ${GDAL_LIBRARIES})

