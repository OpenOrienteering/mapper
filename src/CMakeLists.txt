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

include(../3rd-party/clipper.cmake)

configure_file(mapper_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/mapper_config.h.tmp")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different mapper_config.h.tmp mapper_config.h
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(Mapper_SRCS
 ../3rd-party/clipper/cpp/clipper.cpp

 main.cpp
 util.cpp
 undo.cpp
 qbezier.cpp
 matrix.cpp

 main_window.cpp
 main_window_home_screen.cpp
 print_dock_widget.cpp
 settings.cpp
 settings_dialog.cpp

 map.cpp
 map_widget.cpp
 map_editor.cpp
 map_undo.cpp
 map_dialog_new.cpp
 map_dialog_scale.cpp
 georeferencing.cpp
 georeferencing_dialog.cpp

 map_color.cpp
 color_dock_widget.cpp

 symbol.cpp
 symbol_dock_widget.cpp
 symbol_setting_dialog.cpp
 symbol_properties_widget.cpp
 symbol_point_editor.cpp
 symbol_point.cpp
 symbol_line.cpp
 symbol_area.cpp
 symbol_text.cpp
 symbol_combined.cpp
 renderable.cpp
 renderable_implementation.cpp
 object.cpp
 object_text.cpp
 path_coord.cpp

 template.cpp
 template_image.cpp
 template_gps.cpp
 template_map.cpp
 template_dock_widget.cpp
 template_position_dock_widget.cpp
 template_adjust.cpp
 template_tool_move.cpp
 template_tool_paint.cpp

 tool_edit.cpp
 tool_draw_line_and_area.cpp
 tool_draw_point.cpp
 tool_draw_path.cpp
 tool_draw_circle.cpp
 tool_draw_rectangle.cpp
 tool_draw_text.cpp
 tool_cut.cpp
 tool_cut_hole.cpp
 tool_rotate.cpp
 tool_scale.cpp
 tool_measure.cpp
 tool_boolean.cpp

 gps_coordinates.cpp
 gps_track.cpp
 dxfparser.cpp

 file_format.cpp
 file_format_native.cpp
 file_format_ocad8.cpp
)

set(Mapper_MOC_INPUT
 color_dock_widget.h
 georeferencing.h
 georeferencing_dialog.h
 main_window.h
 main_window_home_screen.h
 map.h
 map_color.h
 map_dialog_new.h
 map_dialog_scale.h
 map_editor.h
 map_undo.h
 map_widget.h
 print_dock_widget.h
 settings.h
 settings_dialog.h
 symbol.h
 symbol_area.h
 symbol_combined.h
 symbol_dock_widget.h
 symbol_line.h
 symbol_point.h
 symbol_point_editor.h
 symbol_properties_widget.h
 symbol_setting_dialog.h
 symbol_text.h
 template.h
 template_adjust.h
 template_dock_widget.h
 template_gps.h
 template_image.h
 template_map.h
 template_position_dock_widget.h
 template_tool_move.h
 template_tool_paint.h
 tool_cut.h
 tool_cut_hole.h
 tool_draw_circle.h
 tool_draw_line_and_area.h
 tool_draw_path.h
 tool_draw_point.h
 tool_draw_rectangle.h
 tool_draw_text.h
 tool_edit.h
 tool_measure.h
 tool_rotate.h
 tool_scale.h
 undo.h
)

if(Mapper_XML_FORMAT)
	list(APPEND Mapper_SRCS file_format_xml.cpp)
endif(Mapper_XML_FORMAT)

qt4_wrap_cpp(Mapper_MOC ${Mapper_MOC_INPUT})

# Resources (from project root)

set(Mapper_RESOURCES ${Mapper_RESOURCES} ${PROJECT_SOURCE_DIR}/resources.qrc)
qt4_add_resources(Mapper_RESOURCES_RCC ${Mapper_RESOURCES} OPTIONS -no-compress)

if(MINGW)
	enable_language(RC)
	Quirks_CMake_RC_Compiler()
	configure_file(mingw/resources.rc.in ${CMAKE_CURRENT_BINARY_DIR}/resources.rc @ONLY)
	configure_file(mingw/control.ico     ${CMAKE_CURRENT_BINARY_DIR}/control.ico  COPYONLY)
	list(APPEND Mapper_SRCS ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
endif(MINGW)

# Mapper executable

add_executable(Mapper
	WIN32
	MACOSX_BUNDLE
	${Mapper_SRCS}
	${Mapper_MOC}
	${Mapper_RESOURCES_RCC}
)
target_link_libraries(Mapper
 ${Mapper_LIBRARIES}
 ${QT_LIBRARIES}
 ${PROJ_LIBRARY}
)
add_dependencies(Mapper
  Mapper_prerequisites
  Mapper_config_h
)

install(TARGETS Mapper
  RUNTIME DESTINATION "${MAPPER_RUNTIME_DESTINATION}"
  BUNDLE DESTINATION "${MAPPER_RUNTIME_DESTINATION}"
)
qt4_translations_sources(
  SOURCES ${Mapper_SRCS}
)

