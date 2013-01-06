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

set(Mapper_Common_SRCS
 ../3rd-party/clipper/cpp/clipper.cpp

 global.cpp
 util.cpp
 util_pie_menu.cpp
 util_task_dialog.cpp
 util_translation.cpp
 mapper_resource.cpp
 undo.cpp
 qbezier.cpp
 matrix.cpp
 transformation.cpp

 main_window.cpp
 settings.cpp
 settings_dialog.cpp

 map.cpp
 map_part.cpp
 map_widget.cpp
 map_editor.cpp
 map_editor_activity.cpp
 map_undo.cpp
 map_dialog_new.cpp
 map_dialog_scale.cpp
 map_dialog_rotate.cpp
 map_grid.cpp
 georeferencing.cpp
 georeferencing_dialog.cpp

 map_color.cpp
 color_dock_widget.cpp

 symbol.cpp
 symbol_dock_widget.cpp
 symbol_dialog_replace.cpp
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
 template_dialog_reopen.cpp
 template_dock_widget.cpp
 template_position_dock_widget.cpp
 template_adjust.cpp
 template_tool_move.cpp
 template_tool_paint.cpp

 tool.cpp
 tool_helpers.cpp
 tool_pan.cpp
 tool_edit.cpp
 tool_edit_point.cpp
 tool_edit_line.cpp
 tool_draw_line_and_area.cpp
 tool_draw_point.cpp
 tool_draw_path.cpp
 tool_draw_circle.cpp
 tool_draw_rectangle.cpp
 tool_draw_text.cpp
 tool_cut.cpp
 tool_cut_hole.cpp
 tool_rotate.cpp
 tool_rotate_pattern.cpp
 tool_scale.cpp
 tool_measure.cpp
 tool_boolean.cpp

 gps_coordinates.cpp
 gps_track.cpp
 dxfparser.cpp

 file_format.cpp
 file_format_native.cpp
 file_format_ocad8.cpp
 file_format_xml.cpp
 
 core/map_printer.cpp
 
 gui/color_dialog.cpp
 gui/home_screen_controller.cpp
 gui/print_tool.cpp
 gui/print_widget.cpp
 
 gui/widgets/color_dropdown.cpp
 gui/widgets/home_screen_widget.cpp
)

set(Mapper_Common_MOC_INPUT
 color_dock_widget.h
 georeferencing.h
 georeferencing_dialog.h
 main_window.h
 map.h
 map_dialog_new.h
 map_dialog_scale.h
 map_dialog_rotate.h
 map_editor.h
 map_editor_activity.h
 map_grid.h
 map_undo.h
 map_widget.h
 settings.h
 settings_dialog.h
 symbol.h
 symbol_area.h
 symbol_combined.h
 symbol_dialog_replace.h
 symbol_dock_widget.h
 symbol_line.h
 symbol_point.h
 symbol_point_editor.h
 symbol_properties_widget.h
 symbol_setting_dialog.h
 symbol_text.h
 template.h
 template_adjust.h
 template_dialog_reopen.h
 template_dock_widget.h
 template_gps.h
 template_image.h
 template_map.h
 template_position_dock_widget.h
 template_tool_move.h
 template_tool_paint.h
 tool.h
 tool_cut.h
 tool_cut_hole.h
 tool_draw_circle.h
 tool_draw_line_and_area.h
 tool_draw_path.h
 tool_draw_point.h
 tool_draw_rectangle.h
 tool_draw_text.h
 tool_edit.h
 tool_edit_point.h
 tool_edit_line.h
 tool_helpers.h
 tool_measure.h
 tool_pan.h
 tool_rotate.h
 tool_rotate_pattern.h
 tool_scale.h
 undo.h
 util_pie_menu.h
 util_task_dialog.h
 
 core/map_printer.h
 
 gui/color_dialog.h
 gui/home_screen_controller.h
 gui/print_tool.h
 gui/print_widget.h
 
 gui/widgets/color_dropdown.h
 gui/widgets/home_screen_widget.h
)

qt4_wrap_cpp(Mapper_Common_MOC ${Mapper_Common_MOC_INPUT})

# Resources (from project root)

set(Mapper_RESOURCES ${Mapper_RESOURCES} ${PROJECT_SOURCE_DIR}/resources.qrc)
qt4_add_resources(Mapper_RESOURCES_RCC ${Mapper_RESOURCES} OPTIONS -no-compress)

# Mapper common: static library of full runtime 
# (To be used by Mapper executable and by system tests.)

add_library(Mapper_Common STATIC
  ${Mapper_Common_SRCS}
  ${Mapper_Common_MOC}
  ${Mapper_RESOURCES_RCC}
)
add_dependencies(Mapper_Common
  Mapper_prerequisites
)

qt4_translations_sources(
  SOURCES ${Mapper_Common_SRCS}
)


# Mapper executable

set(Mapper_SRCS
  main.cpp
)
 
if(MINGW)
	enable_language(RC)
	Quirks_CMake_RC_Compiler()
	configure_file(mingw/resources.rc.in ${CMAKE_CURRENT_BINARY_DIR}/resources.rc @ONLY)
	configure_file(mingw/control.ico     ${CMAKE_CURRENT_BINARY_DIR}/control.ico  COPYONLY)
	list(APPEND Mapper_SRCS ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
endif(MINGW)

add_executable(Mapper
  WIN32
  MACOSX_BUNDLE
  ${Mapper_SRCS}
)
target_link_libraries(Mapper
  Mapper_Common
  libocad
  QtSingleApplication
  ${PROJ_LIBRARY}
  ${QT_LIBRARIES}
)

install(TARGETS Mapper
  RUNTIME DESTINATION "${MAPPER_RUNTIME_DESTINATION}"
  BUNDLE DESTINATION "${MAPPER_RUNTIME_DESTINATION}"
)

# Mapper.pro file

set(MAPPER_PRO_GENERATOR "Generated in src/CMakeLists.txt from src/qmake/oo-mapper.pro.in")
string(REPLACE ";" " \\\n  src/" MAPPER_PRO_HEADERS "src/${Mapper_Common_MOC_INPUT}")
string(REPLACE ";" " \\\n  src/" MAPPER_PRO_SOURCES "src/${Mapper_Common_SRCS}")
configure_file(qmake/oo-mapper.pro.in ${PROJECT_SOURCE_DIR}/oo-mapper.pro @ONLY)