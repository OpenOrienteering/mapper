#
#    Copyright 2012-2014 Thomas Schöps
#    Copyright 2012-2024 Kai Pastor
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
find_package(Qt5Widgets REQUIRED)

if(ANDROID)
	find_package(Qt5AndroidExtras REQUIRED)
else()
	find_package(Qt5Network REQUIRED)
	find_package(Qt5PrintSupport REQUIRED)
endif()

set(CMAKE_AUTOMOC ON)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


# Build configuration

configure_file(mapper_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/mapper_config.h.tmp")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different mapper_config.h.tmp mapper_config.h
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
add_library(mapper-config INTERFACE)
target_include_directories(mapper-config  INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")


# Scaling icon engine

set(SCALING_ICON_ENGINE_SOURCES
  gui/scaling_icon_engine.cpp
  gui/scaling_icon_engine_plugin.cpp
  gui/scaling_icon_engine_plugin.json
)
set(SCALING_ICON_ENGINE_DEFINITIONS
  PUBLIC
    SCALING_ICON_ENGINE_PLUGIN
  PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_USE_QSTRINGBUILDER
    QT_STATICPLUGIN
)
set(SCALING_ICON_ENGINE_LINK_LIBRARIES
  PUBLIC
    Qt5::Gui
)

add_library(scaling-icon-engine STATIC  ${SCALING_ICON_ENGINE_SOURCES})
target_compile_definitions(scaling-icon-engine  ${SCALING_ICON_ENGINE_DEFINITIONS})
target_link_libraries(scaling-icon-engine  ${SCALING_ICON_ENGINE_LINK_LIBRARIES})


# Resources (from project root)

list(APPEND Mapper_RESOURCES "${PROJECT_SOURCE_DIR}/resources.qrc")
qt5_add_resources(Mapper_RESOURCES_RCC ${Mapper_RESOURCES} OPTIONS -no-compress)


# Mapper common: static library of full runtime 
# (To be used by Mapper executable and by system tests.)

set(Mapper_Common_SRCS
  global.cpp
  mapper_resource.cpp
  settings.cpp
  
  core/app_permissions.cpp
  core/autosave.cpp
  core/crs_template.cpp
  core/crs_template_implementation.cpp
  core/georeferencing.cpp
  core/latlon.cpp
  core/map.cpp
  core/map_color.cpp
  core/map_coord.cpp
  core/map_grid.cpp
  core/map_information.cpp
  core/map_part.cpp
  core/map_printer.cpp
  core/map_view.cpp
  core/path_coord.cpp
  core/storage_location.cpp
  core/track.cpp
  core/virtual_coord_vector.cpp
  core/virtual_path.cpp
  
  core/objects/boolean_tool.cpp
  core/objects/object.cpp
  core/objects/object_mover.cpp
  core/objects/object_query.cpp
  core/objects/symbol_rule_set.cpp
  core/objects/text_object.cpp
  
  core/renderables/renderable.cpp
  core/renderables/renderable_implementation.cpp
  
  core/symbols/area_symbol.cpp
  core/symbols/combined_symbol.cpp
  core/symbols/html_symbol_report.cpp
  core/symbols/line_symbol.cpp
  core/symbols/point_symbol.cpp
  core/symbols/symbol.cpp
  core/symbols/symbol_icon_decorator.cpp
  core/symbols/text_symbol.cpp
  
  fileformats/course_file_format.cpp
  fileformats/file_format.cpp
  fileformats/file_format_registry.cpp
  fileformats/file_import_export.cpp
  fileformats/iof_course_export.cpp
  fileformats/kml_course_export.cpp
  fileformats/ocd_file_export.cpp
  fileformats/ocd_file_format.cpp
  fileformats/ocd_file_import.cpp
  fileformats/ocd_georef_fields.cpp
  fileformats/ocd_icon.cpp
  fileformats/ocd_parameter_stream_reader.cpp
  fileformats/ocd_types.cpp
  fileformats/simple_course_export.cpp
  fileformats/xml_file_format.cpp
  
  gui/about_dialog.cpp
  gui/autosave_dialog.cpp
  gui/color_dialog.cpp
  gui/configure_grid_dialog.cpp
  gui/file_dialog.cpp
  gui/georeferencing_dialog.cpp
  gui/home_screen_controller.cpp
  gui/main_window.cpp
  gui/main_window_controller.cpp
  gui/modifier_key.cpp
  gui/print_progress_dialog.cpp
  gui/print_tool.cpp
  gui/print_widget.cpp
  gui/select_crs_dialog.cpp
  gui/settings_dialog.cpp
  gui/simple_course_dialog.cpp
  gui/task_dialog.cpp
  gui/text_browser_dialog.cpp
  gui/touch_cursor.cpp
  gui/util_gui.cpp
  
  gui/map/new_map_dialog.cpp
  gui/map/map_dialog_scale.cpp
  gui/map/map_editor.cpp
  gui/map/map_editor_activity.cpp
  gui/map/map_find_feature.cpp
  gui/map/map_information_dialog.cpp
  gui/map/map_notes.cpp
  gui/map/map_widget.cpp
  gui/map/rotate_map_dialog.cpp
  gui/map/stretch_map_dialog.cpp
  
  gui/symbols/area_symbol_settings.cpp
  gui/symbols/combined_symbol_settings.cpp
  gui/symbols/icon_properties_widget.cpp
  gui/symbols/line_symbol_settings.cpp
  gui/symbols/point_symbol_editor_widget.cpp
  gui/symbols/point_symbol_settings.cpp
  gui/symbols/symbol_properties_widget.cpp
  gui/symbols/symbol_replacement.cpp
  gui/symbols/symbol_replacement_dialog.cpp
  gui/symbols/symbol_report_feature.cpp
  gui/symbols/symbol_setting_dialog.cpp
  gui/symbols/text_symbol_settings.cpp
  
  gui/widgets/action_grid_bar.cpp
  gui/widgets/color_dropdown.cpp
  gui/widgets/color_list_widget.cpp
  gui/widgets/color_wheel_widget.cpp
  gui/widgets/compass_display.cpp
  gui/widgets/crs_param_widgets.cpp
  gui/widgets/crs_selector.cpp
  gui/widgets/editor_settings_page.cpp
  gui/widgets/general_settings_page.cpp
  gui/widgets/home_screen_widget.cpp
  gui/widgets/key_button_bar.cpp
  gui/widgets/mapper_proxystyle.cpp
  gui/widgets/measure_widget.cpp
  gui/widgets/paint_on_template_settings_page.cpp
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
  gui/widgets/text_browser.cpp
  gui/widgets/toast.cpp
  
  templates/paint_on_template_feature.cpp
  templates/paint_on_template_tool.cpp
  templates/template.cpp
  templates/template_adjust.cpp
  templates/template_dialog_reopen.cpp
  templates/template_image.cpp
  templates/template_image_open_dialog.cpp
  templates/template_map.cpp
  templates/template_placeholder.cpp
  templates/template_position_dock_widget.cpp
  templates/template_positioning_dialog.cpp
  templates/template_table_model.cpp
  templates/template_tool_move.cpp
  templates/template_track.cpp
  templates/world_file.cpp
  
  tools/box_zoom_tool.cpp
  tools/cut_tool.cpp
  tools/cut_hole_tool.cpp
  tools/cutout_operation.cpp
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
  tools/object_selector.cpp
  tools/pan_tool.cpp
  tools/point_handles.cpp
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
  
  util/encoding.cpp
  util/item_delegates.cpp
  util/key_value_container.cpp
  util/mapper_service_proxy.cpp
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
  core/image_transparency_fixup.h
  core/objects/object_operations.h
  core/renderables/renderable.h
  core/renderables/renderable_implementation.h
  
  fileformats/file_import_export.h  # translations
  fileformats/ocd_file_import.h     # translations
  fileformats/ocd_types.h
  fileformats/ocd_types_v8.h
  fileformats/ocd_types_v9.h
  fileformats/ocd_types_v10.h
  fileformats/ocd_types_v11.h
  fileformats/ocd_types_v12.h
  fileformats/xml_file_format_p.h
  
  gui/map/map_editor_p.h
  
  templates/world_file.h
  
  util/backports.h
)

add_library(Mapper_Common STATIC
  ${Mapper_Common_SRCS}
  ${Mapper_Common_HEADERS}   # for IDE
  ${Mapper_RESOURCES_RCC}
)
add_dependencies(Mapper_Common
  Mapper_prerequisites
)
target_link_libraries(Mapper_Common
  Polyclipping::Polyclipping
  ${PROJ_LIBRARIES}
  Qt5::Widgets
)
foreach(lib
  cove
  mapper-config
  mapper-gdal
  mapper-sensors
  printsupport
  scaling-icon-engine
  Qt5::AndroidExtras
  Qt5::Network
)
	if(TARGET ${lib})
		target_link_libraries(Mapper_Common ${lib})
	endif()
endforeach()
target_compile_definitions(Mapper_Common PRIVATE
  QT_NO_CAST_FROM_ASCII
  QT_NO_CAST_TO_ASCII
  QT_USE_QSTRINGBUILDER
  # MAPPER_ENABLE_COMPATIBILITY
)
target_compile_definitions(Mapper_Common INTERFACE
  MAPPER_COMMON_LIB
)


mapper_translations_sources(${Mapper_Common_SRCS} ${Mapper_Common_HEADERS})


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

if(ANDROID)
	add_library(Mapper SHARED ${Mapper_SRCS})
else()
	add_executable(Mapper WIN32 MACOSX_BUNDLE ${Mapper_SRCS})
	target_compile_definitions(Mapper PRIVATE MAPPER_USE_QTSINGLEAPPLICATION)
	target_link_libraries(Mapper QtSingleApplication)
endif()

target_link_libraries(Mapper
  Mapper_Common
)
target_compile_definitions(Mapper PRIVATE
  QT_NO_CAST_FROM_ASCII
  QT_NO_CAST_TO_ASCII
  QT_USE_QSTRINGBUILDER
)
if(APPLE)
	add_custom_command(
	  TARGET Mapper
	  POST_BUILD
	  COMMAND plutil -replace NSHighResolutionCapable -bool true
	    $<TARGET_BUNDLE_DIR:Mapper>/Contents/Info.plist
	)
endif()

install(TARGETS Mapper
  RUNTIME DESTINATION "${MAPPER_RUNTIME_DESTINATION}"
  BUNDLE DESTINATION "${MAPPER_RUNTIME_DESTINATION}"   # macOS
  LIBRARY DESTINATION "${MAPPER_RUNTIME_DESTINATION}"  # Android
)


# Java sources for Android
# This target's sources will be build with the APK.

add_custom_target(Mapper_Android SOURCES
  android/java/org/openorienteering/mapper/MapperActivity.java
  android/java/org/openorienteering/mapper/MapperService.java
)


# Workaround Qt private include dir issue
# Cf. https://bugreports.qt.io/browse/QTBUG-37417

set(PRIVATE_MODULES Core Gui)
if(WIN32)
	list(APPEND PRIVATE_MODULES PrintSupport)
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
	target_include_directories(Mapper_Common SYSTEM PRIVATE ${${qt5_module}_PRIVATE_INCLUDE_DIRS})
endforeach()
