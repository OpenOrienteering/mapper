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

DEPENDPATH  += qmake
INCLUDEPATH += qmake

QT += core gui widgets printsupport network xml
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
DEFINES += \"CLIPPER_VERSION='\\"6.1.3a\\"'\"
DEFINES += \"MAPPER_HELP_NAMESPACE='\\"openorienteering.mapper-$${Mapper_VERSION_MAJOR}.$${Mapper_VERSION_MINOR}.$${Mapper_VERSION_PATCH}.help\\"'\"

# Input
HEADERS += \
  qmake\mapper_config.h \
  color_dock_widget.h \
  compass.h \
  file_format_ocad8_p.h \
  file_format_xml_p.h \
  file_import_export.h \
  gps_display.h \
  gps_temporary_markers.h \
  gps_track_recorder.h \
  map.h \
  map_dialog_new.h \
  map_dialog_scale.h \
  map_dialog_rotate.h \
  map_editor.h \
  map_editor_p.h \
  map_editor_activity.h \
  object_undo.h \
  map_widget.h \
  settings.h \
  symbol_area.h \
  symbol_combined.h \
  symbol_dialog_replace.h \
  symbol_line.h \
  symbol_point.h \
  symbol_point_editor.h \
  symbol_properties_widget.h \
  symbol_setting_dialog.h \
  symbol_text.h \
  template.h \
  template_adjust.h \
  template_dialog_reopen.h \
  template_track.h \
  template_image.h \
  template_map.h \
  template_position_dock_widget.h \
  template_tool_move.h \
  template_tool_paint.h \
  tool.h \
  tool_base.h \
  tool_cut.h \
  tool_cut_hole.h \
  tool_cutout.h \
  tool_distribute_points.h \
  tool_draw_circle.h \
  tool_draw_freehand.h \
  tool_draw_line_and_area.h \
  tool_draw_path.h \
  tool_draw_point.h \
  tool_draw_point_gps.h \
  tool_draw_rectangle.h \
  tool_draw_text.h \
  tool_edit.h \
  tool_edit_point.h \
  tool_edit_line.h \
  tool_fill.h \
  tool_helpers.h \
  tool_pan.h \
  tool_rotate.h \
  tool_rotate_pattern.h \
  tool_scale.h \
  undo_manager.h \
  util_task_dialog.h \
  core/autosave_p.h \
  core/georeferencing.h \
  core/map_printer.h \
  core/map_view.h \
  fileformats/ocd_file_export.h \
  fileformats/ocd_file_import.h \
  gui/about_dialog.h \
  gui/autosave_dialog.h \
  gui/color_dialog.h \
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
  gui/text_browser_dialog.h \
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
  gui/widgets/tags_widget.h \
  gui/widgets/template_list_widget.h \
  gui/widgets/text_alignment_widget.h \
  util/item_delegates.h \
  util/overriding_shortcut.h \
  util/recording_translator.h \
  core/crs_template.h \
  core/crs_template_implementation.h \
  core/image_transparency_fixup.h \
  core/latlon.h \
  core/map_coord.h \
  core/map_grid.h \
  core/path_coord.h \
  core/virtual_path.cpp \
  core/virtual_coord_vector.h \
  fileformats/ocd_file_format.h \
  fileformats/ocd_types.h \
  fileformats/ocd_types_v8.h \
  fileformats/ocd_types_v9.h \
  fileformats/ocd_types_v10.h \
  fileformats/ocd_types_v11.h \
  fileformats/ocd_types_v12.h \
  gui/point_handles.h \
  util/backports.h \
  util/scoped_signals_blocker.h \
  map_part.h \
  map_part_undo.h \
  object_operations.h \
  renderable.h \
  renderable_implementation.h \
  symbol.h \
  undo.h \
  util_gui.h

SOURCES += \
  main.cpp \
  core/autosave.cpp \
  core/crs_template.cpp \
  core/crs_template_implementation.cpp \
  core/georeferencing.cpp \
  core/latlon.cpp \
  core/map_color.cpp \
  core/map_coord.cpp \
  core/map_grid.cpp \
  core/map_printer.cpp \
  core/map_view.cpp \
  core/path_coord.cpp \
  core/storage_location.cpp \
  core/virtual_path.cpp \
  core/virtual_coord_vector.cpp \
  global.cpp \
  util.cpp \
  util_task_dialog.cpp \
  util_translation.cpp \
  mapper_resource.cpp \
  undo.cpp \
  undo_manager.cpp \
  matrix.cpp \
  transformation.cpp \
  settings.cpp \
  map.cpp \
  map_part.cpp \
  map_part_undo.cpp \
  map_widget.cpp \
  touch_cursor.cpp \
  map_editor.cpp \
  map_editor_activity.cpp \
  object_undo.cpp \
  map_dialog_new.cpp \
  map_dialog_scale.cpp \
  map_dialog_rotate.cpp \
  color_dock_widget.cpp \
  symbol.cpp \
  symbol_dialog_replace.cpp \
  symbol_setting_dialog.cpp \
  symbol_properties_widget.cpp \
  symbol_point_editor.cpp \
  symbol_point.cpp \
  symbol_line.cpp \
  symbol_area.cpp \
  symbol_text.cpp \
  symbol_combined.cpp \
  renderable.cpp \
  renderable_implementation.cpp \
  object.cpp \
  object_text.cpp \
  template.cpp \
  template_image.cpp \
  template_track.cpp \
  template_map.cpp \
  template_dialog_reopen.cpp \
  template_position_dock_widget.cpp \
  template_adjust.cpp \
  template_tool_move.cpp \
  template_tool_paint.cpp \
  tool.cpp \
  tool_base.cpp \
  tool_helpers.cpp \
  tool_pan.cpp \
  tool_edit.cpp \
  tool_edit_point.cpp \
  tool_edit_line.cpp \
  tool_distribute_points.cpp \
  tool_draw_line_and_area.cpp \
  tool_draw_point.cpp \
  tool_draw_point_gps.cpp \
  tool_draw_path.cpp \
  tool_draw_circle.cpp \
  tool_draw_rectangle.cpp \
  tool_draw_freehand.cpp \
  tool_draw_text.cpp \
  tool_cut.cpp \
  tool_cut_hole.cpp \
  tool_cutout.cpp \
  tool_rotate.cpp \
  tool_rotate_pattern.cpp \
  tool_scale.cpp \
  tool_boolean.cpp \
  tool_fill.cpp \
  gps_track.cpp \
  gps_display.cpp \
  gps_temporary_markers.cpp \
  gps_track_recorder.cpp \
  dxfparser.cpp \
  compass.cpp \
  file_format.cpp \
  file_format_registry.cpp \
  file_import_export.cpp \
  file_format_native.cpp \
  file_format_ocad8.cpp \
  file_format_xml.cpp \
  fileformats/ocd_file_export.cpp \
  fileformats/ocd_file_format.cpp \
  fileformats/ocd_file_import.cpp \
  fileformats/ocd_types.cpp \
  gui/about_dialog.cpp \
  gui/autosave_dialog.cpp \
  gui/color_dialog.cpp \
  gui/georeferencing_dialog.cpp \
  gui/home_screen_controller.cpp \
  gui/main_window.cpp \
  gui/main_window_controller.cpp \
  gui/configure_grid_dialog.cpp \
  gui/modifier_key.cpp \
  gui/point_handles.cpp \
  gui/print_progress_dialog.cpp \
  gui/print_tool.cpp \
  gui/print_widget.cpp \
  gui/select_crs_dialog.cpp \
  gui/settings_dialog.cpp \
  gui/text_browser_dialog.cpp \
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
  gui/widgets/tags_widget.cpp \
  gui/widgets/template_list_widget.cpp \
  gui/widgets/text_alignment_widget.cpp \
  util/item_delegates.cpp \
  util/overriding_shortcut.cpp \
  util/recording_translator.cpp \
  util/scoped_signals_blocker.cpp \
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
    lib/libQt5Xml.so \
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
  HEADERS -= file_format_native.h
  SOURCES -= file_format_native.cpp
}
