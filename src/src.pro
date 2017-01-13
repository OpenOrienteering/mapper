######################################################################
# Generated in CMakeLists.txt from src.pro.in
######################################################################

TEMPLATE = app
TARGET   = Mapper
CONFIG  += c++11
CONFIG  -= debug_and_release
DEFINES *= QT_USE_QSTRINGBUILDER QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII

include(../oo-mapper-version.pri)
include($$OUT_PWD/../prerequisites.pri)
LIBS *= -lpolyclipping -lqtsingleapplication -locd
win32: LIBS *= -lproj-9
else:  LIBS *= -lproj

DEPENDPATH  += qmake "$$PWD"
INCLUDEPATH += qmake "$$PWD"

QT += core gui widgets printsupport network
android: QT += core-private gui-private
win32:   QT += core-private gui-private printsupport-private

# Defines. Use fancy quotation marks to be able to define strings with spaces.
CONFIG(debug, release|debug) {
	DEFINES += APP_VERSION=\"'\\"Debug $${Mapper_VERSION_MAJOR}.$${Mapper_VERSION_MINOR}.$${Mapper_VERSION_PATCH}\\"'\" \
	           MAPPER_DEVELOPMENT_BUILD \
	           MAPPER_DEVELOPMENT_RES_DIR=\"'\\"$$clean_path($$OUT_PWD/..)\\"'\"
	osx:  DEFINES += QT_QTASSISTANT_EXECUTABLE=\"'\\"$$replace(QMAKE_QMAKE, qmake, Assistant.app/Contents/MacOS/Assistant)\\"'\"
	else: DEFINES += QT_QTASSISTANT_EXECUTABLE=\"'\\"$$replace(QMAKE_QMAKE, qmake, assistant)\\"'\"
}
else {
	DEFINES += APP_VERSION=\"'\\"$${Mapper_VERSION_MAJOR}.$${Mapper_VERSION_MINOR}.$${Mapper_VERSION_PATCH}\\"'\" \
	           QT_NO_DEBUG QT_NO_DEBUG_OUTPUT
}
DEFINES += \"MAPPER_HELP_NAMESPACE='\\"openorienteering.mapper-$${Mapper_VERSION_MAJOR}.$${Mapper_VERSION_MINOR}.$${Mapper_VERSION_PATCH}.help\\"'\"

# Input
HEADERS += \
  qmake\mapper_config.h \
  settings.h \
  core/autosave_p.h \
  core/georeferencing.h \
  core/map.h \
  core/map_printer.h \
  core/map_view.h \
  core/symbols/area_symbol.h \
  core/symbols/combined_symbol.h \
  core/symbols/line_symbol.h \
  core/symbols/point_symbol.h \
  core/symbols/text_symbol.h \
  fileformats/ocad8_file_format_p.h \
  fileformats/xml_file_format_p.h \
  fileformats/file_import_export.h \
  fileformats/ocd_file_export.h \
  fileformats/ocd_file_import.h \
  gui/about_dialog.h \
  gui/autosave_dialog.h \
  gui/color_dialog.h \
  gui/color_dock_widget.h \
  gui/configure_grid_dialog.h \
  gui/georeferencing_dialog.h \
  gui/home_screen_controller.h \
  gui/main_window.h \
  gui/main_window_controller.h \
  gui/print_progress_dialog.h \
  gui/print_tool.h \
  gui/print_widget.h \
  gui/select_crs_dialog.h \
  gui/settings_dialog.h \
  gui/task_dialog.h \
  gui/text_browser_dialog.h \
  gui/map/map_dialog_new.h \
  gui/map/map_dialog_rotate.h \
  gui/map/map_dialog_scale.h \
  gui/map/map_editor.h \
  gui/map/map_editor_activity.h \
  gui/map/map_editor_p.h \
  gui/map/map_widget.h \
  gui/symbols/point_symbol_editor_widget.h \
  gui/symbols/replace_symbol_set_dialog.h \
  gui/symbols/symbol_properties_widget.h \
  gui/symbols/symbol_setting_dialog.h \
  gui/widgets/action_grid_bar.h \
  gui/widgets/color_dropdown.h \
  gui/widgets/compass_display.h \
  gui/widgets/crs_param_widgets.h \
  gui/widgets/crs_selector.h \
  gui/widgets/editor_settings_page.h \
  gui/widgets/general_settings_page.h \
  gui/widgets/home_screen_widget.h \
  gui/widgets/key_button_bar.h \
  gui/widgets/mapper_proxystyle.h \
  gui/widgets/measure_widget.h \
  gui/widgets/pie_menu.h \
  gui/widgets/segmented_button_layout.h \
  gui/widgets/settings_page.h \
  gui/widgets/symbol_dropdown.h \
  gui/widgets/symbol_render_widget.h \
  gui/widgets/symbol_tooltip.h \
  gui/widgets/symbol_widget.h \
  gui/widgets/tag_select_widget.h \
  gui/widgets/tags_widget.h \
  gui/widgets/template_list_widget.h \
  gui/widgets/text_alignment_widget.h \
  sensors/compass.h \
  sensors/gps_display.h \
  sensors/gps_temporary_markers.h \
  sensors/gps_track_recorder.h \
  templates/template.h \
  templates/template_adjust.h \
  templates/template_dialog_reopen.h \
  templates/template_image.h \
  templates/template_map.h \
  templates/template_position_dock_widget.h \
  templates/template_tool_move.h \
  templates/template_tool_paint.h \
  templates/template_track.h \
  tools/cut_hole_tool.h \
  tools/cut_tool.h \
  tools/cutout_tool.h \
  tools/distribute_points_tool.h \
  tools/draw_circle_tool.h \
  tools/draw_freehand_tool.h \
  tools/draw_line_and_area_tool.h \
  tools/draw_path_tool.h \
  tools/draw_point_tool.h \
  tools/draw_point_gps_tool.h \
  tools/draw_rectangle_tool.h \
  tools/draw_text_tool.h \
  tools/edit_line_tool.h \
  tools/edit_point_tool.h \
  tools/edit_tool.h \
  tools/fill_tool.h \
  tools/pan_tool.h \
  tools/rotate_tool.h \
  tools/rotate_pattern_tool.h \
  tools/scale_tool.h \
  tools/text_object_editor_helper.h \
  tools/tool.h \
  tools/tool_base.h \
  tools/tool_helpers.h \
  undo/object_undo.h \
  undo/undo_manager.h \
  util/item_delegates.h \
  util/overriding_shortcut.h \
  util/recording_translator.h \
  core/crs_template.h \
  core/crs_template_implementation.h \
  core/image_transparency_fixup.h \
  core/latlon.h \
  core/map_coord.h \
  core/map_grid.h \
  core/map_part.h \
  core/path_coord.h \
  core/virtual_coord_vector.h \
  core/virtual_path.cpp \
  core/objects/object_operations.h \
  core/renderables/renderable.h \
  core/renderables/renderable_implementation.h \
  core/symbols/symbol.h \
  fileformats/ocd_file_format.h \
  fileformats/ocd_types.h \
  fileformats/ocd_types_v8.h \
  fileformats/ocd_types_v9.h \
  fileformats/ocd_types_v10.h \
  fileformats/ocd_types_v11.h \
  fileformats/ocd_types_v12.h \
  gui/point_handles.h \
  gui/util_gui.h \
  undo/map_part_undo.h \
  undo/undo.h \
  util/backports.h \
  util/memory.h \
  util/scoped_signals_blocker.h

SOURCES += \
  main.cpp \
  global.cpp \
  mapper_resource.cpp \
  settings.cpp \
  core/autosave.cpp \
  core/crs_template.cpp \
  core/crs_template_implementation.cpp \
  core/georeferencing.cpp \
  core/latlon.cpp \
  core/map.cpp \
  core/map_color.cpp \
  core/map_coord.cpp \
  core/map_grid.cpp \
  core/map_part.cpp \
  core/map_printer.cpp \
  core/map_view.cpp \
  core/path_coord.cpp \
  core/storage_location.cpp \
  core/virtual_coord_vector.cpp \
  core/virtual_path.cpp \
  core/objects/object.cpp \
  core/objects/object_query.cpp \
  core/objects/text_object.cpp \
  core/renderables/renderable.cpp \
  core/renderables/renderable_implementation.cpp \
  core/symbols/area_symbol.cpp \
  core/symbols/combined_symbol.cpp \
  core/symbols/line_symbol.cpp \
  core/symbols/point_symbol.cpp \
  core/symbols/symbol.cpp \
  core/symbols/text_symbol.cpp \
  fileformats/file_format.cpp \
  fileformats/file_format_registry.cpp \
  fileformats/file_import_export.cpp \
  fileformats/native_file_format.cpp \
  fileformats/ocad8_file_format.cpp \
  fileformats/ocd_file_export.cpp \
  fileformats/ocd_file_format.cpp \
  fileformats/ocd_file_import.cpp \
  fileformats/ocd_types.cpp \
  fileformats/xml_file_format.cpp \
  gui/about_dialog.cpp \
  gui/autosave_dialog.cpp \
  gui/color_dialog.cpp \
  gui/color_dock_widget.cpp \
  gui/configure_grid_dialog.cpp \
  gui/georeferencing_dialog.cpp \
  gui/home_screen_controller.cpp \
  gui/main_window.cpp \
  gui/main_window_controller.cpp \
  gui/modifier_key.cpp \
  gui/point_handles.cpp \
  gui/print_progress_dialog.cpp \
  gui/print_tool.cpp \
  gui/print_widget.cpp \
  gui/select_crs_dialog.cpp \
  gui/settings_dialog.cpp \
  gui/task_dialog.cpp \
  gui/text_browser_dialog.cpp \
  gui/touch_cursor.cpp \
  gui/map/map_dialog_new.cpp \
  gui/map/map_dialog_rotate.cpp \
  gui/map/map_dialog_scale.cpp \
  gui/map/map_editor.cpp \
  gui/map/map_editor_activity.cpp \
  gui/map/map_widget.cpp \
  gui/symbols/point_symbol_editor_widget.cpp \
  gui/symbols/replace_symbol_set_dialog.cpp \
  gui/symbols/symbol_properties_widget.cpp \
  gui/symbols/symbol_setting_dialog.cpp \
  gui/widgets/action_grid_bar.cpp \
  gui/widgets/color_dropdown.cpp \
  gui/widgets/compass_display.cpp \
  gui/widgets/crs_param_widgets.cpp \
  gui/widgets/crs_selector.cpp \
  gui/widgets/editor_settings_page.cpp \
  gui/widgets/general_settings_page.cpp \
  gui/widgets/home_screen_widget.cpp \
  gui/widgets/key_button_bar.cpp \
  gui/widgets/mapper_proxystyle.cpp \
  gui/widgets/measure_widget.cpp \
  gui/widgets/pie_menu.cpp \
  gui/widgets/segmented_button_layout.cpp \
  gui/widgets/settings_page.cpp \
  gui/widgets/symbol_dropdown.cpp \
  gui/widgets/symbol_render_widget.cpp \
  gui/widgets/symbol_tooltip.cpp \
  gui/widgets/symbol_widget.cpp \
  gui/widgets/tag_select_widget.cpp \
  gui/widgets/tags_widget.cpp \
  gui/widgets/template_list_widget.cpp \
  gui/widgets/text_alignment_widget.cpp \
  sensors/compass.cpp \
  sensors/gps_display.cpp \
  sensors/gps_temporary_markers.cpp \
  sensors/gps_track.cpp \
  sensors/gps_track_recorder.cpp \
  templates/template.cpp \
  templates/template_adjust.cpp \
  templates/template_dialog_reopen.cpp \
  templates/template_image.cpp \
  templates/template_map.cpp \
  templates/template_position_dock_widget.cpp \
  templates/template_tool_move.cpp \
  templates/template_tool_paint.cpp \
  templates/template_track.cpp \
  tools/boolean_tool.cpp \
  tools/cut_tool.cpp \
  tools/cut_hole_tool.cpp \
  tools/cutout_tool.cpp \
  tools/distribute_points_tool.cpp \
  tools/draw_line_and_area_tool.cpp \
  tools/draw_point_tool.cpp \
  tools/draw_point_gps_tool.cpp \
  tools/draw_path_tool.cpp \
  tools/draw_circle_tool.cpp \
  tools/draw_rectangle_tool.cpp \
  tools/draw_freehand_tool.cpp \
  tools/draw_text_tool.cpp \
  tools/edit_tool.cpp \
  tools/edit_point_tool.cpp \
  tools/edit_line_tool.cpp \
  tools/fill_tool.cpp \
  tools/pan_tool.cpp \
  tools/rotate_tool.cpp \
  tools/rotate_pattern_tool.cpp \
  tools/scale_tool.cpp \
  tools/text_object_editor_helper.cpp \
  tools/tool.cpp \
  tools/tool_base.cpp \
  tools/tool_helpers.cpp \
  undo/map_part_undo.cpp \
  undo/object_undo.cpp \
  undo/undo.cpp \
  undo/undo_manager.cpp \
  util/dxfparser.cpp \
  util/encoding.cpp \
  util/item_delegates.cpp \
  util/matrix.cpp \
  util/overriding_shortcut.cpp \
  util/recording_translator.cpp \
  util/scoped_signals_blocker.cpp \
  util/transformation.cpp \
  util/translation_util.cpp \
  util/util.cpp \
  util/xml_stream_util.cpp

RESOURCES += \
  ../resources.qrc

CONFIG(debug, release|debug) {
  RESOURCES += ../examples/autosave-example.qrc
}

OTHER_FILES += \
  CMakeLists.txt \
  src.pro.in

android {
  # Android package template customization
  ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../android

  EXPECTED_VERSION = $$Mapper_VERSION_MAJOR\.$$Mapper_VERSION_MINOR\.$$Mapper_VERSION_PATCH
  !system(grep "$$EXPECTED_VERSION" "$$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml") {
      error(The version name in AndroidManifest.xml does not match the version in oo-mapper-version.pri.)
  }

  # Minimized Qt deployment depedencies.
  # Order matters here. Rename the variable, observe the app start, and watch
  # for "Added shared lib" to see the default order of basic Qt libraries and
  # plugins. Other plugins must be added by handed, e.g. image formats.
  # This comes together with explicit specification in AndroidManifest.xml
  ANDROID_DEPLOYMENT_DEPENDENCIES = \
    lib/libQt5Core.so \
    jar/QtAndroid-bundled.jar \
    jar/QtAndroidAccessibility-bundled.jar \
    lib/libQt5Gui.so \
    plugins/platforms/android/libqtforandroid.so \
    plugins/imageformats/libqgif.so \
    plugins/imageformats/libqicns.so \
    plugins/imageformats/libqico.so \
    plugins/imageformats/libqjp2.so \
    plugins/imageformats/libqjpeg.so \
    plugins/imageformats/libqtiff.so \
    plugins/imageformats/libqwebp.so \
    lib/libQt5Widgets.so \
    lib/libQt5Sensors.so \
    lib/libQt5Positioning.so \
    lib/libQt5AndroidExtras.so \
    jar/QtSensors-bundled.jar \
    plugins/sensors/libqtsensors_android.so \
    jar/QtPositioning-bundled.jar \
    plugins/position/libqtposition_android.so \
    # END

  # Do not use qtsingleapplication
  LIBS -= -lqtsingleapplication
  
  # Network not needed
  QT -= network
  
  # Printing not needed
  QT -= printsupport
  
  # Use sensors, positioning and extra modules
  QT += sensors positioning androidextras
  
  # Add examples as resource
  RESOURCES += ../examples/examples.qrc
  
  # Remove legacy code
  DEFINES += NO_NATIVE_FILE_FORMAT
  HEADERS -= fileformats/native_file_format.h
  SOURCES -= fileformats/native_file_format.cpp
}
