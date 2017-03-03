#
#    Copyright 2012-2014 Thomas Schöps
#    Copyright 2012-2017 Kai Pastor
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

if(ANDROID)
	find_package(Qt5AndroidExtras REQUIRED)
else()
	find_package(Qt5Network)
endif()

set(CMAKE_AUTOMOC ON)

if(Mapper_PACKAGE_QT AND UNIX AND NOT APPLE AND NOT Mapper_DEVELOPMENT_BUILD)
	set(MAPPER_USE_QT_CONF_QRC 1)
endif()

configure_file(mapper_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/mapper_config.h.tmp")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different mapper_config.h.tmp mapper_config.h
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
include_directories(AFTER "../3rd-party/qbezier/src") # Always, last

set(Mapper_Common_SRCS
  global.cpp
  mapper_resource.cpp
  settings.cpp
  
  core/autosave.cpp
  core/crs_template.cpp
  core/crs_template_implementation.cpp
  core/georeferencing.cpp
  core/latlon.cpp
  core/map.cpp
  core/map_color.cpp
  core/map_coord.cpp
  core/map_grid.cpp
  core/map_part.cpp
  core/map_printer.cpp
  core/map_view.cpp
  core/path_coord.cpp
  core/storage_location.cpp
  core/virtual_coord_vector.cpp
  core/virtual_path.cpp
  
  core/objects/boolean_tool.cpp
  core/objects/object.cpp
  core/objects/object_query.cpp
  core/objects/text_object.cpp
  
  core/renderables/renderable.cpp
  core/renderables/renderable_implementation.cpp
  
  core/symbols/area_symbol.cpp
  core/symbols/combined_symbol.cpp
  core/symbols/line_symbol.cpp
  core/symbols/point_symbol.cpp
  core/symbols/symbol.cpp
  core/symbols/text_symbol.cpp
  
  fileformats/file_format.cpp
  fileformats/file_format_registry.cpp
  fileformats/file_import_export.cpp
  fileformats/native_file_format.cpp
  fileformats/ocad8_file_format.cpp
  fileformats/ocd_file_export.cpp
  fileformats/ocd_file_format.cpp
  fileformats/ocd_file_import.cpp
  fileformats/ocd_types.cpp
  fileformats/xml_file_format.cpp
  
  gui/about_dialog.cpp
  gui/autosave_dialog.cpp
  gui/color_dialog.cpp
  gui/color_dock_widget.cpp
  gui/configure_grid_dialog.cpp
  gui/georeferencing_dialog.cpp
  gui/home_screen_controller.cpp
  gui/main_window.cpp
  gui/main_window_controller.cpp
  gui/modifier_key.cpp
  gui/point_handles.cpp
  gui/print_progress_dialog.cpp
  gui/print_tool.cpp
  gui/print_widget.cpp
  gui/select_crs_dialog.cpp
  gui/settings_dialog.cpp
  gui/task_dialog.cpp
  gui/text_browser_dialog.cpp
  gui/touch_cursor.cpp
  
  gui/map/map_dialog_new.cpp
  gui/map/map_dialog_rotate.cpp
  gui/map/map_dialog_scale.cpp
  gui/map/map_editor.cpp
  gui/map/map_editor_activity.cpp
  gui/map/map_widget.cpp
  
  gui/symbols/area_symbol_settings.cpp
  gui/symbols/combined_symbol_settings.cpp
  gui/symbols/line_symbol_settings.cpp
  gui/symbols/point_symbol_editor_widget.cpp
  gui/symbols/point_symbol_settings.cpp
  gui/symbols/replace_symbol_set_dialog.cpp
  gui/symbols/symbol_properties_widget.cpp
  gui/symbols/symbol_setting_dialog.cpp
  gui/symbols/text_symbol_settings.cpp
  
  gui/widgets/action_grid_bar.cpp
  gui/widgets/color_dropdown.cpp
  gui/widgets/compass_display.cpp
  gui/widgets/crs_param_widgets.cpp
  gui/widgets/crs_selector.cpp
  gui/widgets/editor_settings_page.cpp
  gui/widgets/general_settings_page.cpp
  gui/widgets/home_screen_widget.cpp
  gui/widgets/key_button_bar.cpp
  gui/widgets/mapper_proxystyle.cpp
  gui/widgets/measure_widget.cpp
  gui/widgets/pie_menu.cpp
  gui/widgets/segmented_button_layout.cpp
  gui/widgets/settings_page.cpp
  gui/widgets/symbol_dropdown.cpp
  gui/widgets/symbol_render_widget.cpp
  gui/widgets/symbol_tooltip.cpp
  gui/widgets/symbol_widget.cpp
  gui/widgets/tag_select_widget.cpp
  gui/widgets/tags_widget.cpp
  gui/widgets/template_list_widget.cpp
  gui/widgets/text_alignment_widget.cpp
  
  sensors/compass.cpp
  sensors/gps_display.cpp
  sensors/gps_temporary_markers.cpp
  sensors/gps_track.cpp
  sensors/gps_track_recorder.cpp
  
  templates/template.cpp
  templates/template_adjust.cpp
  templates/template_dialog_reopen.cpp
  templates/template_image.cpp
  templates/template_map.cpp
  templates/template_position_dock_widget.cpp
  templates/template_positioning_dialog.cpp
  templates/template_tool_move.cpp
  templates/template_tool_paint.cpp
  templates/template_track.cpp
  templates/world_file.cpp
  
  tools/cut_tool.cpp
  tools/cut_hole_tool.cpp
  tools/cutout_tool.cpp
  tools/distribute_points_tool.cpp
  tools/draw_line_and_area_tool.cpp
  tools/draw_point_tool.cpp
  tools/draw_point_gps_tool.cpp
  tools/draw_path_tool.cpp
  tools/draw_circle_tool.cpp
  tools/draw_rectangle_tool.cpp
  tools/draw_freehand_tool.cpp
  tools/draw_text_tool.cpp
  tools/edit_tool.cpp
  tools/edit_point_tool.cpp
  tools/edit_line_tool.cpp
  tools/fill_tool.cpp
  tools/pan_tool.cpp
  tools/rotate_tool.cpp
  tools/rotate_pattern_tool.cpp
  tools/scale_tool.cpp
  tools/text_object_editor_helper.cpp
  tools/tool.cpp
  tools/tool_base.cpp
  tools/tool_helpers.cpp
  
  undo/map_part_undo.cpp
  undo/object_undo.cpp
  undo/undo.cpp
  undo/undo_manager.cpp
  
  util/dxfparser.cpp
  util/encoding.cpp
  util/item_delegates.cpp
  util/matrix.cpp
  util/overriding_shortcut.cpp
  util/recording_translator.cpp
  util/scoped_signals_blocker.cpp
  util/transformation.cpp
  util/translation_util.cpp
  util/util.cpp
  util/xml_stream_util.cpp
)

# Extra header to be shown in the IDE or to be translated
set(Mapper_Common_HEADERS
  core/autosave_p.h
  core/image_transparency_fixup.h
  core/objects/object_operations.h
  core/renderables/renderable.h
  core/renderables/renderable_implementation.h
  
  fileformats/file_import_export.h  # translations
  fileformats/ocad8_file_format_p.h
  fileformats/ocd_file_import.h     # translations
  fileformats/ocd_types.h
  fileformats/ocd_types_v8.h
  fileformats/ocd_types_v9.h
  fileformats/ocd_types_v10.h
  fileformats/ocd_types_v11.h
  fileformats/ocd_types_v12.h
  fileformats/xml_file_format_p.h
  
  gui/util_gui.h
  gui/map/map_editor_p.h
  
  templates/world_file.h
  
  util/backports.h
)


# Resources (from project root)

if(MAPPER_USE_QT_CONF_QRC)
	set(Mapper_RESOURCES ${Mapper_RESOURCES} ${PROJECT_BINARY_DIR}/3rd-party/qt5/qt.conf.qrc)
endif()
set(Mapper_RESOURCES ${Mapper_RESOURCES} ${PROJECT_SOURCE_DIR}/resources.qrc)
qt5_add_resources(Mapper_RESOURCES_RCC ${Mapper_RESOURCES} OPTIONS -no-compress)


# Mapper common: static library of full runtime 
# (To be used by Mapper executable and by system tests.)

add_library(Mapper_Common STATIC
  ${Mapper_Common_SRCS}
  ${Mapper_Common_HEADERS}   # for IDE
  ${Mapper_RESOURCES_RCC}
)
add_dependencies(Mapper_Common
  Mapper_prerequisites
)
target_link_libraries(Mapper_Common
  libocad
  Polyclipping::Polyclipping
  PROJ4::proj
  Qt5::Widgets
)
foreach(lib Qt5::Network Qt5::AndroidExtras printsupport mapper-gdal)
	if(TARGET ${lib})
		target_link_libraries(Mapper_Common ${lib})
	endif()
endforeach()
target_compile_definitions(Mapper_Common PRIVATE
  QT_NO_CAST_FROM_ASCII
  QT_NO_CAST_TO_ASCII
  QT_USE_QSTRINGBUILDER
)

qt5_translations_sources(
  SOURCES ${Mapper_Common_SRCS} ${Mapper_Common_HEADERS}
)


# Mapper executable

set(Mapper_SRCS
  main.cpp
)
 
if(WIN32)
	enable_language(RC)
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
	target_link_libraries(Mapper QtSingleApplication)
endif()

target_link_libraries(Mapper
  Mapper_Common
  doc-licensing
)
target_compile_definitions(Mapper PRIVATE
  QT_NO_CAST_FROM_ASCII
  QT_NO_CAST_TO_ASCII
  QT_USE_QSTRINGBUILDER
)

install(TARGETS Mapper
  RUNTIME DESTINATION "${MAPPER_RUNTIME_DESTINATION}"
  BUNDLE DESTINATION "${MAPPER_RUNTIME_DESTINATION}"
)

if(Mapper_BUILD_QT)
	write_qt_conf()
elseif(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
	file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
endif()


# Workaround Qt private include dir issue
# Cf. https://bugreports.qt.io/browse/QTBUG-37417

if(WIN32)
	set(PRIVATE_MODULES Core Gui PrintSupport)
else()
	set(PRIVATE_MODULES)
endif()
foreach(module ${PRIVATE_MODULES})
	set(qt_module Qt${module})
	set(qt5_module Qt5${module})
	if("${${qt5_module}_PRIVATE_INCLUDE_DIRS}" STREQUAL "")
		foreach(base_dir ${${qt5_module}_INCLUDE_DIRS})
			if("${base_dir}" MATCHES "/${qt_module}\$")
				list(APPEND ${qt5_module}_PRIVATE_INCLUDE_DIRS "${base_dir}/${${qt5_module}_VERSION}/${qt_module}")
			endif()
		endforeach()
	endif()
	target_include_directories(Mapper_Common PRIVATE ${${qt5_module}_PRIVATE_INCLUDE_DIRS})
endforeach()


# qmake configuration

foreach(cpp_file ${Mapper_Common_SRCS})
	string(REPLACE ".cpp" ".h" h_file "${cpp_file}")
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${h_file}")
		list(APPEND Mapper_Common_HEADERS "${h_file}")
	endif()
endforeach()
list(REMOVE_DUPLICATES Mapper_Common_HEADERS)
list(SORT Mapper_Common_HEADERS)
list(SORT Mapper_Common_SRCS)

set(MAPPER_PRO_GENERATOR "Generated in CMakeLists.txt from src.pro.in")
string(REPLACE ";" " \\\n  " MAPPER_PRO_HEADERS "${Mapper_Common_HEADERS}")
string(REPLACE ";" " \\\n  " MAPPER_PRO_SOURCES "${Mapper_Common_SRCS}")
configure_file(src.pro.in src.pro.tmp @ONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/src.pro.tmp" "${CMAKE_CURRENT_SOURCE_DIR}/src.pro")
