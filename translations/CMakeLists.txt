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
 
if(NOT ${QT4_FOUND})
	find_package(Qt4 QUIET REQUIRED)
endif()

if(QT_LUPDATE_EXECUTABLE MATCHES NOTFOUND)
	message(WARNING "lupdate executable not found. "
	  "You will not be able to update the translations from the source code.")
endif()

if(QT_LRELEASE_EXECUTABLE MATCHES NOTFOUND)
	message(FATAL_ERROR "lrelease executable not found. "
	  "Cannot build the translations.")
endif()

set(Mapper_TRANS
 OpenOrienteering_de.ts
 OpenOrienteering_ja.ts
 OpenOrienteering_lv.ts
 OpenOrienteering_nb.ts
 OpenOrienteering_sv.ts
 OpenOrienteering_uk.ts
 CACHE INTERNAL "Translation files"
)
qt4_add_translation(Mapper_QM ${Mapper_TRANS})

set(Mapper_TRANS_TEMPLATE
 OpenOrienteering_template.ts
 CACHE INTERNAL "Translation template file"
)

set(Mapper_TRANS_DIR
 ${CMAKE_CURRENT_SOURCE_DIR}
 CACHE INTERNAL "Translations build directory"
)

if(Mapper_TRANSLATIONS_EMBEDDED)
	set(Mapper_TRANS_QRC "${CMAKE_CURRENT_BINARY_DIR}/translations.qrc")
	file(WRITE "${Mapper_TRANS_QRC}.in"
	  "<!DOCTYPE RCC><RCC version=\"1.0\">\n <qresource>\n")
	foreach(_qm_file ${Mapper_QM})
		get_filename_component(_qm_filename "${_qm_file}" NAME)
		file(APPEND "${Mapper_TRANS_QRC}.in"
		  "  <file alias=\"translations/${_qm_filename}\">${_qm_file}</file>\n")
	endforeach(_qm_file)
	file(APPEND "${Mapper_TRANS_QRC}.in"
	  " </qresource>\n</RCC>\n")
	execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${Mapper_TRANS_QRC}.in" "${Mapper_TRANS_QRC}")
	qt4_add_resources(Mapper_TRANSLATIONS_RCC "${Mapper_TRANS_QRC}" OPTIONS -no-compress)

	# A library is a top-level target, the generated source file isn't.
	add_library(Mapper_translations STATIC ${Mapper_TRANSLATIONS_RCC})
	add_dependencies(Mapper_translations ${Mapper_QM})

	set(Mapper_LIBRARIES 
	  Mapper_translations 
	  ${Mapper_LIBRARIES}
	  PARENT_SCOPE)

else()
	# A target which builds all translations.
	add_custom_target(Mapper_translations ALL DEPENDS ${Mapper_QM})

if(WIN32)
	unset(MAPPER_QT_TRANSLATIONS CACHE)
	foreach(_mapper_trans ${Mapper_TRANS})
		get_filename_component(_qt_translation ${_mapper_trans} NAME_WE)
		string(REPLACE OpenOrienteering qt _qt_translation ${_qt_translation})
		set(_qt_translation_path "${QT_TRANSLATIONS_DIR}/${_qt_translation}.qm")
		if(EXISTS "${_qt_translation_path}")
			message("   ${_qt_translation} translation - found")
			list(APPEND MAPPER_QT_TRANSLATIONS "${_qt_translation_path}")
		else()
			message("   ${_qt_translation} translation - not found")
		endif()
if(QT_QTASSISTANT_EXECUTABLE)
		string(REPLACE qt qt_help _qt_translation ${_qt_translation})
		set(_qt_translation_path "${QT_TRANSLATIONS_DIR}/${_qt_translation}.qm")
		if(EXISTS "${_qt_translation_path}")
			message("   ${_qt_translation} translation - found")
			list(APPEND MAPPER_QT_TRANSLATIONS "${_qt_translation_path}")
		else()
			message("   ${_qt_translation} translation - not found")
		endif()
		string(REPLACE qt_help assistant _qt_translation ${_qt_translation})
		set(_qt_translation_path "${QT_TRANSLATIONS_DIR}/${_qt_translation}.qm")
		if(EXISTS "${_qt_translation_path}")
			message("   ${_qt_translation} translation - found")
			list(APPEND MAPPER_QT_TRANSLATIONS "${_qt_translation_path}")
		else()
			message("   ${_qt_translation} translation - not found")
		endif()
endif(QT_QTASSISTANT_EXECUTABLE)
	endforeach(_mapper_trans)
endif(WIN32)
	install(
	  FILES ${Mapper_TRANS} ${MAPPER_QT_TRANSLATIONS} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}/translations")

endif(Mapper_TRANSLATIONS_EMBEDDED)


# A target which updates all translations.

add_custom_target(${PROJECT_NAME}_translations_update)


# A temporary file which lists the source files to be translated.

set(${PROJECT_NAME}_TRANSLATIONS_LISTFILE ${CMAKE_CURRENT_BINARY_DIR}/translations_sourcefiles.txt
	CACHE INTERNAL "A temporary file which lists the source files to be translated")
file(WRITE "${${PROJECT_NAME}_TRANSLATIONS_LISTFILE}")

#
# A macro for registering translations sources and creating update targets.
#
# Synopsis:
#
# QT4_TRANSLATIONS_SOURCES(SOURCES a.cpp b.cpp)
#
macro(QT4_TRANSLATIONS_SOURCES)
	set(_DO_SOURCES FALSE)
	foreach(_arg ${Mapper_TRANS} ${Mapper_TRANS_TEMPLATE} ${ARGN})
		if("${_arg}" STREQUAL "SOURCES")
			set(_DO_SOURCES TRUE)
		elseif(_DO_SOURCES)
			get_source_file_property(_abs_path ${_arg} LOCATION)
			file(APPEND "${${PROJECT_NAME}_TRANSLATIONS_LISTFILE}" "${_abs_path}\n")
		else()
			get_filename_component(_ts_filename ${_arg} NAME_WE)
			add_custom_target(${PROJECT_NAME}_${_ts_filename}_update
			  COMMAND ${QT_LUPDATE_EXECUTABLE} @${${PROJECT_NAME}_TRANSLATIONS_LISTFILE} -ts ${Mapper_TRANS_DIR}/${_arg}
			  DEPENDS ${${PROJECT_NAME}_TRANSLATIONS_LISTFILE}
			  VERBATIM)
			add_dependencies(${PROJECT_NAME}_translations_update ${PROJECT_NAME}_${_ts_filename}_update)
		endif()
	endforeach()
endmacro()

