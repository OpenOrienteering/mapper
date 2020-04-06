#!/bin/sh -e

#    Copyright 2017, 2018 Kai Pastor
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
 

# This is a wrapper for code quality tools supported by CMake.
# 
# It adds these benefits over direct use of the tools:
# 
# - It provides a pattern for filenames on which the tools are to be
#   applied. This limits the noise in relevant diagnostic output and
#   cuts build times by skipping files which are not of interest.
# - It allows changing pattern and arguments without forcing a complete
#   rebuild of all sources handled by this compiler (which is always
#   triggered by changes to the CMake variables).
#
# To use this wrapper, set its full path as the program for each check,
# with the actual tool as the first argument, e.g.
#
#   CMAKE_CXX_CLANG_TIDY="/path/to/code-check-wrapper.sh;clang-tidy"
#   CMAKE_CXX_INCLUDE_WHAT_YOU_USE="/path/to/code-check-wrapper.sh;iwyu"
#
# Any other parameter is ignored. So modifying extra parameters can still
# be used to force a full re-run.


PROGRAM=$1
shift

ENABLE_CLANG_TIDY=true
ENABLE_IWYU=true

PATTERN=
for I in \
  action_grid_bar.cpp \
  combined_symbol.cpp \
  configure_grid_dialog.cpp \
  crs_param_widgets.cpp \
  crs_template.cpp \
  crs_template_implementation.cpp \
  duplicate_equals_t.cpp \
  file_dialog.cpp \
  /file_format.cpp \
  file_format_t.cpp \
  file_import_export.cpp \
  georeferencing.cpp \
  georeferencing_dialog.cpp \
  georeferencing_t.cpp \
  key_button_bar.cpp \
  line_symbol.cpp \
  main.cpp \
  /map.cpp \
  map_coord.cpp \
  map_editor.cpp \
  map_find_feature.cpp \
  map_widget.cpp \
  /object.cpp \
  object_mover.cpp \
  object_query.cpp \
  ocd_file_format.cpp \
  ocd_t.cpp \
  overriding_shortcut.cpp \
  point_symbol.cpp \
  print_widget.cpp \
  renderable.cpp \
  renderable_implementation.cpp \
  settings_dialog.cpp \
  /symbol.cpp \
  symbol_replacement.cpp \
  symbol_replacement_dialog.cpp \
  symbol_rule_set.cpp \
  symbol_t.cpp \
  symbol_tooltip.cpp \
  tag_select_widget.cpp \
  /template.cpp \
  template_image.cpp \
  template_image_open_dialog.cpp \
  template_list_widget.cpp \
  template_map.cpp \
  template_placeholder.cpp \
  template_tool \
  template_track.cpp \
  text_brwoser_dialog \
  toast.cpp \
  track_t.cpp \
  /track.cpp \
  undo_manager.cpp \
  /util.cpp \
  /util_gui.cpp \
  world_file.cpp \
  xml_file_format.cpp \
  xml_stream_util.cpp \
  \
  "3rd-party/cove/[^ ]*.cpp" \
  gdal/ \
  ocd \
  src/sensors/ \
  src/tools/ \
  settings \
  # end of patterns
do
  PATTERN="${PATTERN:+$PATTERN\|}$I"
done

if echo "$@" | grep -q "${PATTERN}"; then
	case "${PROGRAM}" in
	*clang-tidy*)
		if ${ENABLE_CLANG_TIDY}; then
			"${PROGRAM}" \
			  "$@" \
			|| exit 1
		fi
		;;
	*iwyu*|*include-what-you-use*)
		if ${ENABLE_IWYU}; then
			"${PROGRAM}" \
			  -Xiwyu --mapping_file=${0%/*}/iwyu-mapper.imp \
			  -Xiwyu --check_also=*_p.h \
			  -Xiwyu --max_line_length=160 \
			  "-DqPrintable(...)=(void(__VA_ARGS__), \"\")" \
			  "-DqUtf8Printable(...)=(void(__VA_ARGS__), \"\")" \
			  "$@" \
			|| exit 1
		fi
		;;
	*)
		"${PROGRAM}" "$@" || exit 1
	esac
else
	true;
fi
