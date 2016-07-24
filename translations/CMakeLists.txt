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
 
message(STATUS "Configuring ${PROJECT_NAME} translations")

find_package(Qt5LinguistTools REQUIRED)

set(Mapper_TRANS
 OpenOrienteering_cs.ts
 OpenOrienteering_de.ts
 OpenOrienteering_en.ts
 OpenOrienteering_es.ts
 OpenOrienteering_et.ts
 OpenOrienteering_fi.ts
 OpenOrienteering_fr.ts
 OpenOrienteering_hu.ts
 OpenOrienteering_it.ts
 OpenOrienteering_ja.ts
 OpenOrienteering_lv.ts
 OpenOrienteering_nb.ts
 OpenOrienteering_nl.ts
 OpenOrienteering_pl.ts
 OpenOrienteering_pt_BR.ts
 OpenOrienteering_ru.ts
 OpenOrienteering_sv.ts
 OpenOrienteering_uk.ts
 CACHE INTERNAL "Translation files"
)
qt5_add_translation(Mapper_QM ${Mapper_TRANS})

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
	file(WRITE "${Mapper_TRANS_QRC}.tmp"
	  "<!DOCTYPE RCC><RCC version=\"1.0\">\n <qresource>\n")
	foreach(_qm_file ${Mapper_QM})
		get_filename_component(_qm_filename "${_qm_file}" NAME)
		file(APPEND "${Mapper_TRANS_QRC}.tmp"
		  "  <file alias=\"translations/${_qm_filename}\">${_qm_file}</file>\n")
	endforeach(_qm_file)
	file(APPEND "${Mapper_TRANS_QRC}.tmp"
	  " </qresource>\n</RCC>\n")
	execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${Mapper_TRANS_QRC}.tmp" "${Mapper_TRANS_QRC}")
	qt5_add_resources(Mapper_TRANSLATIONS_RCC "${Mapper_TRANS_QRC}" OPTIONS -no-compress)

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

	install(
	  FILES ${Mapper_QM}
	  DESTINATION "${MAPPER_DATA_DESTINATION}/translations")

endif(Mapper_TRANSLATIONS_EMBEDDED)

if(APPLE)
	# Localized resources just to enable localized native dialogs.
	foreach(_mapper_trans en ${Mapper_TRANS})
		get_filename_component(_mapper_lang ${_mapper_trans} NAME_WE)
		string(REPLACE OpenOrienteering_ "" LANGUAGE_CODE ${_mapper_lang})
		configure_file(locversion.plist.in ${LANGUAGE_CODE}.lproj/locversion.plist @ONLY)
		install(
		  DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${LANGUAGE_CODE}.lproj"
		  DESTINATION "${MAPPER_DATA_DESTINATION}")
	endforeach()
endif()

# For super-builds (e.g. cross-compilation), wait for the Qt build.
add_dependencies(Mapper_translations Mapper_prerequisites)


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
# QT5_TRANSLATIONS_SOURCES(SOURCES a.cpp b.cpp)
#
macro(QT5_TRANSLATIONS_SOURCES)
	find_package(Qt5LinguistTools REQUIRED QUIET) # Provides Qt5_LUPDATE_EXECUTABLE
	set(_DO_SOURCES FALSE)
	foreach(_arg ${Mapper_TRANS} ${Mapper_TRANS_TEMPLATE} ${ARGN})
		if("${_arg}" STREQUAL "SOURCES")
			set(_DO_SOURCES TRUE)
		elseif(_DO_SOURCES)
			get_source_file_property(_abs_path ${_arg} LOCATION)
			file(APPEND "${${PROJECT_NAME}_TRANSLATIONS_LISTFILE}" "${_abs_path}\n")
		else()
			get_filename_component(_ts_filename ${_arg} NAME_WE)
			set(LUPDATE_OPTIONS -locations absolute)
			if("${_ts_filename}" STREQUAL "OpenOrienteering_en")
				list(APPEND LUPDATE_OPTIONS -pluralonly)
			endif()
			if("${_ts_filename}" STREQUAL "OpenOrienteering_template")
				list(APPEND LUPDATE_OPTIONS -no-obsolete)
			endif()
			if("${_ts_filename}" MATCHES "OpenOrienteering_(et|lv|nb|nl|template)")
				# Qt translations which are not yet available from upstream, as of Qt 5.5.1.
				# Inserted from qt_template.ts into the OpenOrienteering .ts file by
				#   lconvert -sort-contexts -o OpenOrienteering_XX.ts 
				# Need to revert the "obsolete" and "vanished" marker created by lupdate,
				# but prevent deletion of empty "vanished" items.
				set(PRE_LUPDATE_ACTION
				  COMMAND sed -i -e "/<name>Q[A-Z]/,/<context>/s/\\(<translation[^>]*>\\)</\\1REMOVE_ME\\</" ${Mapper_TRANS_DIR}/${_arg}
				)
				set(POST_LUPDATE_ACTION
				  COMMAND sed -i -e "/<name>Q[A-Z]/,/<context>/s/<translation[^>]*>REMOVE_ME</<translation type=\"unfinished\"></" ${Mapper_TRANS_DIR}/${_arg}
				  COMMAND sed -i -e "s/>REMOVE_ME</></g" ${Mapper_TRANS_DIR}/${_arg}
				  COMMAND sed -i -e "/<name>Q[A-Z]/,/<context>/s/ type=.[ov][a-z]*.//g" ${Mapper_TRANS_DIR}/${_arg}
				  COMMAND sed -i -e "/<name>Q[A-Z]/,/<context>/s/\\(<translation\\)\\/>/\\1 type='unfinished'><\\/translation>/g" ${Mapper_TRANS_DIR}/${_arg}
				)
			else()
				set(PRE_LUPDATE_ACTION )
				set(POST_LUPDATE_ACTION
				  COMMAND sed -i -e "/<name>Q[A-Z]/,/<context>/d" ${Mapper_TRANS_DIR}/${_arg}
				)
			endif()
			add_custom_target(${PROJECT_NAME}_${_ts_filename}_update
			  ${PRE_LUPDATE_ACTION}
			  COMMAND ${Qt5_LUPDATE_EXECUTABLE} ${LUPDATE_OPTIONS} @${${PROJECT_NAME}_TRANSLATIONS_LISTFILE} -ts ${Mapper_TRANS_DIR}/${_arg}
			  ${POST_LUPDATE_ACTION}
			  # Reformat for Weblate to minimize git noise
			  COMMAND sed -i -e "1s/\"/'/g" ${Mapper_TRANS_DIR}/${_arg}
			  COMMAND sed -i -e "/<location/s/\\/>$/><\\/location>/" ${Mapper_TRANS_DIR}/${_arg}
			  COMMAND sed -i -e "/<source\\|<translation\\|<[a-z]*comment/,/\\/source>\\|\\/translation>\\|\\/[a-z]*comment>/{s/&apos;/'/g;s/&quot;/\"/g}" ${Mapper_TRANS_DIR}/${_arg}
			  DEPENDS ${${PROJECT_NAME}_TRANSLATIONS_LISTFILE}
			  VERBATIM)
			add_dependencies(${PROJECT_NAME}_translations_update ${PROJECT_NAME}_${_ts_filename}_update)
		endif()
	endforeach()
endmacro()

message(STATUS "Configuring ${PROJECT_NAME} translations - done")
