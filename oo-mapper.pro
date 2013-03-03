######################################################################
# Generated in src/CMakeLists.txt from src/qmake/oo-mapper.pro.in
#####################################################################

TEMPLATE = app
TARGET = Mapper
DEPENDPATH += . src
INCLUDEPATH += . src src/qmake

# Needed for QtSingleApplication
QT += network xml

# Needed when compiling pure C code
QMAKE_CFLAGS += -std=c99

# PROJ.4 library
QMAKE_LIBS += -lproj

# Input
HEADERS += \
  src/color_dock_widget.h \
  src/file_format_ocad8_p.h \
  src/file_format_xml_p.h \
  src/file_import_export.h \
  src/georeferencing.h \
  src/georeferencing_dialog.h \
  src/map.h \
  src/map_dialog_new.h \
  src/map_dialog_scale.h \
  src/map_dialog_rotate.h \
  src/map_editor.h \
  src/map_editor_activity.h \
  src/map_grid.h \
  src/map_undo.h \
  src/map_widget.h \
  src/settings.h \
  src/symbol.h \
  src/symbol_area.h \
  src/symbol_combined.h \
  src/symbol_dialog_replace.h \
  src/symbol_dock_widget.h \
  src/symbol_line.h \
  src/symbol_point.h \
  src/symbol_point_editor.h \
  src/symbol_properties_widget.h \
  src/symbol_setting_dialog.h \
  src/symbol_text.h \
  src/template.h \
  src/template_adjust.h \
  src/template_dialog_reopen.h \
  src/template_dock_widget.h \
  src/template_gps.h \
  src/template_image.h \
  src/template_map.h \
  src/template_position_dock_widget.h \
  src/template_tool_move.h \
  src/template_tool_paint.h \
  src/tool.h \
  src/tool_base.h \
  src/tool_cut.h \
  src/tool_cut_hole.h \
  src/tool_cutout.h \
  src/tool_draw_circle.h \
  src/tool_draw_line_and_area.h \
  src/tool_draw_path.h \
  src/tool_draw_point.h \
  src/tool_draw_rectangle.h \
  src/tool_draw_text.h \
  src/tool_edit.h \
  src/tool_edit_point.h \
  src/tool_edit_line.h \
  src/tool_helpers.h \
  src/tool_pan.h \
  src/tool_rotate.h \
  src/tool_rotate_pattern.h \
  src/tool_scale.h \
  src/undo.h \
  src/util_pie_menu.h \
  src/util_task_dialog.h \
  src/core/map_printer.h \
  src/gui/about_dialog.h \
  src/gui/color_dialog.h \
  src/gui/home_screen_controller.h \
  src/gui/main_window.h \
  src/gui/main_window_controller.h \
  src/gui/print_tool.h \
  src/gui/print_widget.h \
  src/gui/settings_dialog.h \
  src/gui/settings_dialog_p.h \
  src/gui/widgets/color_dropdown.h \
  src/gui/widgets/home_screen_widget.h \
  src/gui/widgets/measure_widget.h \
  3rd-party/qtsingleapplication/src/qtlocalpeer.h \
  3rd-party/qtsingleapplication/src/qtsingleapplication.h \
  libocad/libocad.h \
  libocad/array.h \
  libocad/geometry.h \
  libocad/types.h

SOURCES += \
  src/global.cpp \
  src/util.cpp \
  src/util_pie_menu.cpp \
  src/util_task_dialog.cpp \
  src/util_translation.cpp \
  src/mapper_resource.cpp \
  src/undo.cpp \
  src/qbezier.cpp \
  src/matrix.cpp \
  src/transformation.cpp \
  src/settings.cpp \
  src/map.cpp \
  src/map_part.cpp \
  src/map_widget.cpp \
  src/map_editor.cpp \
  src/map_editor_activity.cpp \
  src/map_undo.cpp \
  src/map_dialog_new.cpp \
  src/map_dialog_scale.cpp \
  src/map_dialog_rotate.cpp \
  src/map_grid.cpp \
  src/georeferencing.cpp \
  src/georeferencing_dialog.cpp \
  src/color_dock_widget.cpp \
  src/symbol.cpp \
  src/symbol_dock_widget.cpp \
  src/symbol_dialog_replace.cpp \
  src/symbol_setting_dialog.cpp \
  src/symbol_properties_widget.cpp \
  src/symbol_point_editor.cpp \
  src/symbol_point.cpp \
  src/symbol_line.cpp \
  src/symbol_area.cpp \
  src/symbol_text.cpp \
  src/symbol_combined.cpp \
  src/renderable.cpp \
  src/renderable_implementation.cpp \
  src/object.cpp \
  src/object_text.cpp \
  src/path_coord.cpp \
  src/template.cpp \
  src/template_image.cpp \
  src/template_gps.cpp \
  src/template_map.cpp \
  src/template_dialog_reopen.cpp \
  src/template_dock_widget.cpp \
  src/template_position_dock_widget.cpp \
  src/template_adjust.cpp \
  src/template_tool_move.cpp \
  src/template_tool_paint.cpp \
  src/tool.cpp \
  src/tool_base.cpp \
  src/tool_helpers.cpp \
  src/tool_pan.cpp \
  src/tool_edit.cpp \
  src/tool_edit_point.cpp \
  src/tool_edit_line.cpp \
  src/tool_draw_line_and_area.cpp \
  src/tool_draw_point.cpp \
  src/tool_draw_path.cpp \
  src/tool_draw_circle.cpp \
  src/tool_draw_rectangle.cpp \
  src/tool_draw_text.cpp \
  src/tool_cut.cpp \
  src/tool_cut_hole.cpp \
  src/tool_cutout.cpp \
  src/tool_rotate.cpp \
  src/tool_rotate_pattern.cpp \
  src/tool_scale.cpp \
  src/tool_boolean.cpp \
  src/gps_coordinates.cpp \
  src/gps_track.cpp \
  src/dxfparser.cpp \
  src/file_format.cpp \
  src/file_format_registry.cpp \
  src/file_import_export.cpp \
  src/file_format_native.cpp \
  src/file_format_ocad8.cpp \
  src/file_format_xml.cpp \
  src/core/map_color.cpp \
  src/core/map_printer.cpp \
  src/gui/about_dialog.cpp \
  src/gui/color_dialog.cpp \
  src/gui/home_screen_controller.cpp \
  src/gui/main_window.cpp \
  src/gui/main_window_controller.cpp \
  src/gui/print_tool.cpp \
  src/gui/print_widget.cpp \
  src/gui/settings_dialog.cpp \
  src/gui/widgets/color_dropdown.cpp \
  src/gui/widgets/home_screen_widget.cpp \
  src/gui/widgets/measure_widget.cpp \
  src/main.cpp \
  3rd-party/qtsingleapplication/src/qtlocalpeer.cpp \
  3rd-party/qtsingleapplication/src/qtsingleapplication.cpp \
  libocad/types.c \
  libocad/array.c \
  libocad/geometry.c \
  libocad/path.c \
  libocad/file.c \
  libocad/setup.c \
  libocad/color.c \
  libocad/ocad_symbol.c \
  libocad/ocad_object.c \
  libocad/string.c

RESOURCES += \
  resources.qrc
