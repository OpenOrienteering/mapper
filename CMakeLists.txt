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
 
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Project declaration

project(Mapper)

SET(Mapper_VERSION_MAJOR 0)
SET(Mapper_VERSION_MINOR 3)
SET(Mapper_VERSION_PATCH 0)
SET(Mapper_VERSION_DISPLAY "${CMAKE_BUILD_TYPE} ${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}")

# Build configuration options

if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
	  "Type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	  FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT Mapper_XML_FORMAT)
	set(Mapper_XML_FORMAT OFF CACHE BOOL
	  "Enable the unmaintained export-only XML format, options are: OFF, ON."
	  FORCE)
endif(NOT Mapper_XML_FORMAT)
if(Mapper_XML_FORMAT)
	add_definitions(-DWITH_MAPPER_XML_FORMAT)
endif()

if(NOT Mapper_TRANSLATIONS_EMBEDDED)
	if(APPLE)
		# For MacOS, the translations will go into the "bundle" by default
		set(Mapper_TRANSLATIONS_EMBEDDED_DEFAULT ON)
	else(APPLE)
		set(Mapper_TRANSLATIONS_EMBEDDED_DEFAULT OFF)
	endif(APPLE)
	set(Mapper_TRANSLATIONS_EMBEDDED ${Mapper_TRANSLATIONS_EMBEDDED_DEFAULT} CACHE BOOL
	  "Embed all translations in the program executable, options are: OFF, ON."
	  FORCE)
endif(NOT Mapper_TRANSLATIONS_EMBEDDED)

# Build definitons

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CMake28Quirks)

configure_file(src/global.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/global.h)

find_package(Qt4 4.6 REQUIRED QtCore QtGui QtNetwork QtXml)
include(${QT_USE_FILE})
Quirks_Qt4_definitions()
Quirks_Qt4_lib_dependencies()

if(CMAKE_CROSSCOMPILING)
	message("-- Crosscompiling, host: ${CMAKE_HOST_SYSTEM_NAME}, target: ${CMAKE_SYSTEM_NAME}")

	# use host system Qt4 tools when cross-compiling
	find_program(QT_LUPDATE_EXECUTABLE lupdate)
	find_program(QT_LRELEASE_EXECUTABLE lrelease)
	find_program(QT_QCOLLECTIONGENERATOR_EXECUTABLE qcollectiongenerator)
endif()

if(QT_LUPDATE_EXECUTABLE MATCHES NOTFOUND)
	message(WARNING "lupdate executable not found. "
	  "You will not be able to update the translations from the source code.")
endif()

if(QT_LRELEASE_EXECUTABLE MATCHES NOTFOUND)
	message(FATAL_ERROR "lrelease executable not found.")
endif()

if(QT_QCOLLECTIONGENERATOR_EXECUTABLE MATCHES NOTFOUND)
	message(FATAL_ERROR "qcollectiongenerator executable not found.")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include(3rd-party/clipper.cmake)
find_package(Proj QUIET)
if(NOT PROJ_FOUND)
	include(3rd-party/proj.cmake)
endif(NOT PROJ_FOUND)
find_package(Proj REQUIRED)

if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-Wno-long-long -Wno-deprecated -Wall -pedantic")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
	set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O2 -s")
	set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG")

	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
	set(CMAKE_C_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_RELEASE})
	set(CMAKE_C_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
else (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")
endif (CMAKE_COMPILER_IS_GNUCXX)

set(OPT_INCLUDES "/opt/local/include")
set(OPT_LIBRARIES "/opt/local/lib")

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${PROJ_INCLUDE_DIR} ${OPT_INCLUDES})
link_directories(${OPT_LIBRARIES})

set(Mapper_SRCS
 libocad/types.c
 libocad/array.c
 libocad/geometry.c
 libocad/path.c
 libocad/file.c
 libocad/color.c
 libocad/setup.c
 libocad/ocad_symbol.c
 libocad/ocad_object.c
 libocad/string.c

 qtsingleapplication/qtlocalpeer.cpp
 qtsingleapplication/qtsingleapplication.cpp
 qtsingleapplication/qtsinglecoreapplication.cpp

 3rd-party/clipper/cpp/clipper.cpp

 src/main.cpp
 src/util.cpp
 src/undo.cpp
 src/qbezier.cpp
 src/matrix.cpp

 src/main_window.cpp
 src/main_window_home_screen.cpp
 src/print_dock_widget.cpp
 src/settings.cpp
 src/settings_dialog.cpp

 src/map.cpp
 src/map_widget.cpp
 src/map_editor.cpp
 src/map_undo.cpp
 src/map_dialog_new.cpp
 src/map_dialog_scale.cpp
 src/georeferencing.cpp
 src/georeferencing_dialog.cpp

 src/map_color.cpp
 src/color_dock_widget.cpp

 src/symbol.cpp
 src/symbol_dock_widget.cpp
 src/symbol_setting_dialog.cpp
 src/symbol_properties_widget.cpp
 src/symbol_point_editor.cpp
 src/symbol_point.cpp
 src/symbol_line.cpp
 src/symbol_area.cpp
 src/symbol_text.cpp
 src/symbol_combined.cpp
 src/renderable.cpp
 src/renderable_implementation.cpp
 src/object.cpp
 src/object_text.cpp
 src/path_coord.cpp

 src/template.cpp
 src/template_image.cpp
 src/template_gps.cpp
 src/template_map.cpp
 src/template_dock_widget.cpp
 src/template_position_dock_widget.cpp
 src/template_adjust.cpp
 src/template_tool_move.cpp
 src/template_tool_paint.cpp

 src/tool_edit.cpp
 src/tool_draw_line_and_area.cpp
 src/tool_draw_point.cpp
 src/tool_draw_path.cpp
 src/tool_draw_circle.cpp
 src/tool_draw_rectangle.cpp
 src/tool_draw_text.cpp
 src/tool_cut.cpp
 src/tool_cut_hole.cpp
 src/tool_rotate.cpp
 src/tool_scale.cpp
 src/tool_measure.cpp
 src/tool_boolean.cpp

 src/gps_coordinates.cpp
 src/gps_track.cpp
 src/dxfparser.cpp

 src/file_format.cpp
 src/file_format_native.cpp
 src/file_format_ocad8.cpp
 src/file_format_xml.cpp
)

set(Mapper_MOC_INPUT
 qtsingleapplication/qtlocalpeer.h
 qtsingleapplication/qtsingleapplication.h
 qtsingleapplication/qtsinglecoreapplication.h

 src/color_dock_widget.h
 src/georeferencing.h
 src/georeferencing_dialog.h
 src/main_window.h
 src/main_window_home_screen.h
 src/map.h
 src/map_color.h
 src/map_dialog_new.h
 src/map_dialog_scale.h
 src/map_editor.h
 src/map_undo.h
 src/map_widget.h
 src/print_dock_widget.h
 src/settings.h
 src/settings_dialog.h
 src/symbol.h
 src/symbol_area.h
 src/symbol_combined.h
 src/symbol_dock_widget.h
 src/symbol_line.h
 src/symbol_point.h
 src/symbol_point_editor.h
 src/symbol_properties_widget.h
 src/symbol_setting_dialog.h
 src/symbol_text.h
 src/template.h
 src/template_adjust.h
 src/template_dock_widget.h
 src/template_gps.h
 src/template_image.h
 src/template_map.h
 src/template_position_dock_widget.h
 src/template_tool_move.h
 src/template_tool_paint.h
 src/tool_cut.h
 src/tool_cut_hole.h
 src/tool_draw_circle.h
 src/tool_draw_line_and_area.h
 src/tool_draw_path.h
 src/tool_draw_point.h
 src/tool_draw_rectangle.h
 src/tool_draw_text.h
 src/tool_edit.h
 src/tool_measure.h
 src/tool_rotate.h
 src/tool_scale.h
 src/undo.h
)

qt4_wrap_cpp(Mapper_MOC ${Mapper_MOC_INPUT})

set(Mapper_TRANS
 translations/OpenOrienteering_de.ts
 translations/OpenOrienteering_ja.ts
 translations/OpenOrienteering_lv.ts
 translations/OpenOrienteering_nb.ts
 translations/OpenOrienteering_sv.ts
 translations/OpenOrienteering_uk.ts
)
set_source_files_properties(${Mapper_TRANS}
  PROPERTIES OUTPUT_LOCATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations
)
qt4_add_translation(Mapper_QM ${Mapper_TRANS})

set(Mapper_TRANS_TEMPLATE
 translations/OpenOrienteering_template.ts
)
include(Qt4_translations)
qt4_translations_sources(
  ${Mapper_TRANS} ${Mapper_TRANS_TEMPLATE}
  SOURCES ${Mapper_SRCS}
)

if(Mapper_TRANSLATIONS_EMBEDDED)
	set(Mapper_TRANS_QRC "${CMAKE_CURRENT_BINARY_DIR}/translations.qrc")
	file(WRITE "${Mapper_TRANS_QRC}"
	  "<!DOCTYPE RCC><RCC version=\"1.0\">\n <qresource>\n")
	foreach(_qm_file ${Mapper_QM})
		get_filename_component(_qm_filename "${_qm_file}" NAME)
		file(APPEND "${Mapper_TRANS_QRC}"
		  "  <file alias=\"translations/${_qm_filename}\">${_qm_file}</file>\n")
	endforeach(_qm_file)
	file(APPEND "${Mapper_TRANS_QRC}"
	  " </qresource>\n</RCC>\n")
	qt4_add_resources(Mapper_TRANSLATIONS_RCC "${Mapper_TRANS_QRC}" OPTIONS -no-compress)
endif(Mapper_TRANSLATIONS_EMBEDDED)

# A target which builds all translations.
add_custom_target(Mapper_translations DEPENDS ${Mapper_QM} ${Mapper_TRANSLATIONS_RCC})

set(Mapper_RESOURCES ${Mapper_RESOURCES} resources.qrc)
qt4_add_resources(Mapper_RESOURCES_RCC ${Mapper_RESOURCES} OPTIONS -no-compress)

set(Mapper_HELP_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/help)
set(Mapper_HELP_COLLECTION ${Mapper_HELP_DIR}/oomaphelpcollection.qhc)
file(GLOB Mapper_HELP_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/images
  ${CMAKE_CURRENT_SOURCE_DIR}/help/html_en 
  ${CMAKE_CURRENT_SOURCE_DIR}/help/html_en/images
)
add_custom_command( 
	OUTPUT    ${Mapper_HELP_COLLECTION}
	COMMAND   ${QT_QCOLLECTIONGENERATOR_EXECUTABLE}
	ARGS      ${CMAKE_CURRENT_SOURCE_DIR}/oomaphelpcollection.qhcp -o ${Mapper_HELP_COLLECTION}
	MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/oomaphelpcollection.qhcp
	DEPENDS    oomaphelp.qhp ${Mapper_HELP_FILES}
	COMMENT   "Compile help collection file"
)
add_custom_target(Mapper_help
  DEPENDS ${Mapper_HELP_COLLECTION}
)

if(MINGW)
	# resource compilation for MinGW
	enable_language(RC)
	if(NOT CMAKE_RC_COMPILER)
		set(CMAKE_RC_COMPILER windres)
	endif()
	Quirks_CMake_RC_Compiler()
	configure_file(resources/resources.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.rc)
	list(APPEND Mapper_SRCS resources/resources.rc)
endif(MINGW)

add_executable(Mapper
	WIN32
	MACOSX_BUNDLE
	${Mapper_SRCS}
	${Mapper_MOC}
	${Mapper_QM}
	${Mapper_HELP_COLLECTION}
	${Mapper_RESOURCES_RCC}
	${Mapper_TRANSLATIONS_RCC}
)

target_link_libraries(Mapper
 ${QT_LIBRARIES}
 ${PROJ_LIBRARY}
)

# FIXME: Packaging for MacOSX needs review on a MacOSX machine
if(NOT APPLE)
include(CPackaging.cmake)
endif()
