#
#    Copyright 2012 Kai Pastor
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

if(NOT ${QT4_FOUND})
	find_package(Qt4 QUIET REQUIRED)
endif()

#
# A target which updates all translations.
#
add_custom_target(${PROJECT_NAME}_translations_update)

#
# A temporary file which lists the source files to be translated.
#
set(${PROJECT_NAME}_TRANSLATIONS_LISTFILE ${CMAKE_CURRENT_BINARY_DIR}/translations_sourcefiles.txt)
file(WRITE "${${PROJECT_NAME}_TRANSLATIONS_LISTFILE}")

#
# A macro for registering translations sources and creating update targets.
# This macro may be used only once in a project.
#
# Synopsis:
#
# QT4_TRANSLATIONS_SOURCES(x_de.ts y_de.ts SOURCES a.cpp b.cpp)
#
macro(QT4_TRANSLATIONS_SOURCES)
	set(_DO_SOURCES FALSE)
	foreach(_arg ${ARGN})
		if("${_arg}" STREQUAL "SOURCES")
			set(_DO_SOURCES TRUE)
		elseif(_DO_SOURCES)
			get_source_file_property(_abs_path ${_arg} LOCATION)
			file(APPEND "${${PROJECT_NAME}_TRANSLATIONS_LISTFILE}" "${_abs_path}\n")
		else()
			get_filename_component(_ts_filename ${_arg} NAME_WE)
			add_custom_target(${PROJECT_NAME}_${_ts_filename}_update
			  COMMAND ${QT_LUPDATE_EXECUTABLE} @${${PROJECT_NAME}_TRANSLATIONS_LISTFILE} -ts ${CMAKE_CURRENT_SOURCE_DIR}/${_arg}
			  DEPENDS ${${PROJECT_NAME}_TRANSLATIONS_LISTFILE}
			  VERBATIM)
			add_dependencies(${PROJECT_NAME}_translations_update ${PROJECT_NAME}_${_ts_filename}_update)
		endif()
	endforeach()
endmacro()

