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
find_package(Qt5SerialPort)
if(ANDROID)
	find_package(Qt5AndroidExtras REQUIRED)
endif()

set(CMAKE_AUTOMOC ON)


# PowershellPositionPlugin
#
# For testing purposes, we build this plugin on all platforms,
# but we register it in main.cpp on WIN32 only.

set(POWERSHELL_POSITION_DEFINITIONS
  PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_USE_QSTRINGBUILDER
    QT_STATICPLUGIN
)
set(POWERSHELL_POSITION_SOURCES )
set(POWERSHELL_POSITION_LINK_LIBRARIES )
if(TARGET Qt5::Positioning)
	list(APPEND POWERSHELL_POSITION_DEFINITIONS
	  PUBLIC MAPPER_USE_POWERSHELL_POSITION_PLUGIN
	)
	list(APPEND POWERSHELL_POSITION_SOURCES
	  powershell_position_plugin.cpp
	  powershell_position_plugin.json
	  powershell_position_source.cpp
	)
	qt5_add_resources(POWERSHELL_POSITION_SOURCES
	  "${CMAKE_CURRENT_SOURCE_DIR}/powershell_position_source.qrc"
	)
	list(APPEND POWERSHELL_POSITION_LINK_LIBRARIES
	  PRIVATE Qt5::Positioning
	)
endif()

add_library(powershell_position_source STATIC  ${POWERSHELL_POSITION_SOURCES})
target_compile_definitions(powershell_position_source  ${POWERSHELL_POSITION_DEFINITIONS})
target_link_libraries(powershell_position_source  ${POWERSHELL_POSITION_LINK_LIBRARIES})


# Mapper sensors

set(MAPPER_SENSORS_SOURCES
  compass.cpp
  gps_display.cpp
  gps_temporary_markers.cpp
  gps_track_recorder.cpp
  sensors_settings_page.cpp
)

set(MAPPER_SENSORS_DEFINITIONS
  PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_USE_QSTRINGBUILDER
)

if(TARGET Qt5::Positioning OR TARGET Qt5::Sensors)
	list(APPEND MAPPER_SENSORS_DEFINITIONS  PUBLIC MAPPER_USE_SENSORS)
endif()

add_library(mapper-sensors STATIC  ${MAPPER_SENSORS_SOURCES})
target_compile_definitions(mapper-sensors  ${MAPPER_SENSORS_DEFINITIONS})
target_include_directories(mapper-sensors  PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(mapper-sensors
  PUBLIC
    powershell_position_source
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)
foreach(lib Qt5::Positioning Qt5::Sensors Qt5::SerialPort Qt5::AndroidExtras)
	if(TARGET ${lib})
		target_link_libraries(mapper-sensors  PRIVATE ${lib})
	endif()
endforeach()


# Translations

mapper_translations_sources(
  ${MAPPER_SENSORS_SOURCES}
  ${POWERSHELL_POSITION_SOURCES}
)
