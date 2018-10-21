#
#    Copyright 2018-2019 Kai Pastor
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

find_package(Qt5Core 5.3 REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Sensors)
find_package(Qt5Positioning)
if(ANDROID)
	find_package(Qt5AndroidExtras REQUIRED)
endif()

set(CMAKE_AUTOMOC ON)

# Extra header to be shown in the IDE (e.g. private) or to be translated
set(MAPPER_SENSORS_HEADERS
  # not needed ATM
)

set(MAPPER_SENSORS_SOURCES
  compass.cpp
  gps_display.cpp
  gps_temporary_markers.cpp
  gps_track_recorder.cpp
)

mapper_translations_sources(
  ${MAPPER_SENSORS_HEADERS}
  ${MAPPER_SENSORS_SOURCES}
)

add_library(mapper-sensors STATIC
  ${MAPPER_SENSORS_HEADERS}
  ${MAPPER_SENSORS_SOURCES}
)

target_compile_definitions(mapper-sensors
  PUBLIC
    MAPPER_USE_SENSORS
  PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_USE_QSTRINGBUILDER
)

target_include_directories(mapper-sensors
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(mapper-sensors
  PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)
foreach(lib Qt5::Positioning Qt5::Sensors Qt5::AndroidExtras)
	if(TARGET ${lib})
		target_link_libraries(mapper-sensors PRIVATE ${lib})
	endif()
endforeach()
