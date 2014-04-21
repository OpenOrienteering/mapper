#
#    Copyright 2012, 2013, 2014 Thomas Schöps, Kai Pastor
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

if(Mapper_PACKAGE_QT AND UNIX AND NOT APPLE AND NOT Mapper_DEVELOPMENT_BUILD)
	set(MAPPER_USE_QT_CONF_QRC 1)
endif()

configure_file(mapper_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/mapper_config.h.tmp")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different mapper_config.h.tmp mapper_config.h
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(AFTER "../3rd-party/qbezier/src") # Always, last

set(Mapper_Common_SRCS
 global.cpp
 util.cpp
 util_task_dialog.cpp
 util_translation.cpp
 mapper_resource.cpp
 undo.cpp
 matrix.cpp
 transformation.cpp

 settings.cpp

 map.cpp
 map_part.cpp
 map_widget.cpp
 touch_cursor.cpp
 map_editor.cpp
 map_editor_activity.cpp
 map_undo.cpp
 map_dialog_new.cpp
 map_dialog_scale.cpp
 map_dialog_rotate.cpp
 map_grid.cpp
 georeferencing.cpp
 georeferencing_dialog.cpp

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
 template_track.cpp
 template_map.cpp
 template_dialog_reopen.cpp
 template_dock_widget.cpp
 template_position_dock_widget.cpp
 template_adjust.cpp
 template_tool_move.cpp
 template_tool_paint.cpp

 tool.cpp
 tool_base.cpp
 tool_helpers.cpp
 tool_pan.cpp
 tool_edit.cpp
 tool_edit_point.cpp
 tool_edit_line.cpp
 tool_distribute_points.cpp
 tool_draw_line_and_area.cpp
 tool_draw_point.cpp
 tool_draw_point_gps.cpp
 tool_draw_path.cpp
 tool_draw_circle.cpp
 tool_draw_rectangle.cpp
 tool_draw_freehand.cpp
 tool_draw_text.cpp
 tool_cut.cpp
 tool_cut_hole.cpp
 tool_cutout.cpp
 tool_rotate.cpp
 tool_rotate_pattern.cpp
 tool_scale.cpp
 tool_boolean.cpp
 tool_fill.cpp

 gps_coordinates.cpp
 gps_track.cpp
 gps_display.cpp
 gps_temporary_markers.cpp
 gps_track_recorder.cpp
 dxfparser.cpp
 
 compass.cpp
 compass_display.cpp

 file_format.cpp
 file_format_registry.cpp
 file_import_export.cpp
 file_format_native.cpp
 file_format_ocad8.cpp
 file_format_xml.cpp
 
 core/auto_save.cpp
 core/map_color.cpp
 core/map_printer.cpp
 core/map_view.cpp
 
 fileformats/ocd_file_format.cpp
 fileformats/ocd_types.cpp
 
 gui/about_dialog.cpp
 gui/color_dialog.cpp
 gui/home_screen_controller.cpp
 gui/main_window.cpp
 gui/main_window_controller.cpp
 gui/modifier_key.cpp
 gui/print_progress_dialog.cpp
 gui/print_tool.cpp
 gui/print_widget.cpp
 gui/settings_dialog.cpp
 
 gui/widgets/action_grid_bar.cpp
 gui/widgets/color_dropdown.cpp
 gui/widgets/home_screen_widget.cpp
 gui/widgets/key_button_bar.cpp
 gui/widgets/mapper_proxystyle.cpp
 gui/widgets/measure_widget.cpp
 gui/widgets/pie_menu.cpp
 gui/widgets/segmented_button_layout.cpp
 gui/widgets/tags_widget.cpp
 
 util/item_delegates.cpp
 util/overriding_shortcut.cpp
 util/recording_translator.cpp
 util/xml_stream_util.cpp
)

set(Mapper_Common_MOC_INPUT
 color_dock_widget.h
 compass.h
 compass_display.h
 file_format_ocad8_p.h
 file_format_xml_p.h
 file_import_export.h
 georeferencing.h
 georeferencing_dialog.h
 gps_display.h
 gps_temporary_markers.h
 gps_track_recorder.h
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
 template_track.h
 template_image.h
 template_map.h
 template_position_dock_widget.h
 template_tool_move.h
 template_tool_paint.h
 tool.h
 tool_base.h
 tool_cut.h
 tool_cut_hole.h
 tool_cutout.h
 tool_distribute_points.h
 tool_draw_circle.h
 tool_draw_freehand.h
 tool_draw_line_and_area.h
 tool_draw_path.h
 tool_draw_point.h
 tool_draw_point_gps.h
 tool_draw_rectangle.h
 tool_draw_text.h
 tool_edit.h
 tool_edit_point.h
 tool_edit_line.h
 tool_fill.h
 tool_helpers.h
 tool_pan.h
 tool_rotate.h
 tool_rotate_pattern.h
 tool_scale.h
 undo.h
 util_task_dialog.h
 
 core/auto_save_p.h
 core/map_printer.h
 
 fileformats/ocd_file_format_p.h
 
 gui/about_dialog.h
 gui/color_dialog.h
 gui/home_screen_controller.h
 gui/main_window.h
 gui/main_window_controller.h
 gui/print_progress_dialog.h
 gui/print_tool.h
 gui/print_widget.h
 gui/settings_dialog.h
 gui/settings_dialog_p.h
 
 gui/widgets/action_grid_bar.h
 gui/widgets/color_dropdown.h
 gui/widgets/home_screen_widget.h
 gui/widgets/key_button_bar.h
 gui/widgets/mapper_proxystyle.h
 gui/widgets/measure_widget.h
 gui/widgets/pie_menu.h
 gui/widgets/segmented_button_layout.h
 gui/widgets/tags_widget.h
 
 util/item_delegates.h
 util/overriding_shortcut.h
 util/recording_translator.h
)

set(Mapper_EXTRA_HEADERS
  core/auto_save_p.h
  core/image_transparency_fixup.h
  fileformats/ocd_file_format_p.h
  fileformats/ocd_types_v8.h
  fileformats/ocd_types_v9.h
  fileformats/ocd_types_v10.h
  fileformats/ocd_types_v11.h
  gui/settings_dialog_p.h
  util/scoped_signals_blocker.h
  file_format_ocad8_p.h
)

qt5_wrap_cpp(Mapper_Common_MOC ${Mapper_Common_MOC_INPUT})

# Resources (from project root)

if(MAPPER_USE_QT_CONF_QRC)
	set(Mapper_RESOURCES ${Mapper_RESOURCES} ${PROJECT_BINARY_DIR}/3rd-party/qt5/qt.conf.qrc)
endif()
set(Mapper_RESOURCES ${Mapper_RESOURCES} ${PROJECT_SOURCE_DIR}/resources.qrc)
qt5_add_resources(Mapper_RESOURCES_RCC ${Mapper_RESOURCES} OPTIONS -no-compress)


# Mapper_Extra_Header: dummy target to indicate extra headers to IDEs

add_custom_target(Mapper_Extras
  SOURCES ${Mapper_EXTRA_HEADERS}
)

# Mapper common: static library of full runtime 
# (To be used by Mapper executable and by system tests.)

add_library(Mapper_Common STATIC
  ${Mapper_Common_SRCS}
  ${Mapper_Common_MOC}
  ${Mapper_RESOURCES_RCC}
)
add_dependencies(Mapper_Common
  Mapper_prerequisites
  Mapper_Extras
)
target_link_libraries(Mapper_Common
  libocad
)

qt5_translations_sources(
  SOURCES ${Mapper_Common_SRCS} ${Mapper_COMMON_MOC_INPUT}
)


# Mapper executable

set(Mapper_SRCS
  main.cpp
)
 
if(WIN32)
	enable_language(RC)
	if(MINGW)
		# Cf. http://public.kitware.com/Bug/view.php?id=12480
		cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)
		# Cf. http://public.kitware.com/Bug/view.php?id=14865
		if(CMAKE_RC_COMPILER MATCHES "windres\$" AND CMAKE_RC_COMPILE_OBJECT MATCHES "/fo")
			include(Platform/${CMAKE_SYSTEM_NAME}-windres)
		endif()
	endif()
	configure_file(mingw/resources.rc.in ${CMAKE_CURRENT_BINARY_DIR}/resources.rc @ONLY)
	configure_file(${PROJECT_SOURCE_DIR}/images/mapper-icon/Mapper.ico ${CMAKE_CURRENT_BINARY_DIR}/Mapper.ico COPYONLY)
	list(APPEND Mapper_SRCS ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
endif()

add_executable(Mapper
  WIN32
  MACOSX_BUNDLE
  ${Mapper_SRCS}
)

if(TARGET QtSingleApplication)
	include_directories("../3rd-party/qtsingleapplication/src")
	target_link_libraries(Mapper QtSingleApplication)
endif()

target_link_libraries(Mapper
  Mapper_Common
  docs-licensing
  ${PROJ_LIBRARY}
  ${QT_LIBRARIES}
  ${CLIPPER_LIBRARY}
)

install(TARGETS Mapper
  RUNTIME DESTINATION "${MAPPER_RUNTIME_DESTINATION}"
  BUNDLE DESTINATION "${MAPPER_RUNTIME_DESTINATION}"
)

if(Mapper_BUILD_QT)
	write_qt_conf()
endif()


# qmake configuration

set(MAPPER_PRO_GENERATOR "Generated in CMakeLists.txt from src.pro.in")
string(REPLACE ";" " \\\n  " MAPPER_PRO_HEADERS "${Mapper_Common_MOC_INPUT}")
string(REPLACE ";" " \\\n  " MAPPER_PRO_SOURCES "${Mapper_Common_SRCS}")
configure_file(${PROJECT_SOURCE_DIR}/src/src.pro.in ${PROJECT_SOURCE_DIR}/src/src.pro @ONLY)
