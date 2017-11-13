#
#    Copyright 2012, 2013, 2014 Thomas Schöps
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

if(NOT PROJECT_NAME)
	# FIXME Used by legacy qmake build for Android
	cmake_minimum_required(VERSION 3.0)
	project(Mapper_Manual NONE)
	set(MAPPER_DATA_DESTINATION /assets)
	enable_language(CXX)
else()
	message(STATUS "Configuring ${PROJECT_NAME} user manual")
endif()


set(Mapper_MANUAL_QTHELP_default ON)
if(ANDROID)
	set(Mapper_MANUAL_QTHELP_default OFF)
endif()
option(Mapper_MANUAL_QTHELP
  "Provide the manual as Qt Help collection"
  ${Mapper_MANUAL_QTHELP_default}
)


# Qt provides a broken Qt5::qcollectiongenerator when crosscompiling.
if (CMAKE_CROSSCOMPILING AND NOT TARGET Qt5::qcollectiongenerator)
	find_program(Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE
	  NAMES qcollectiongenerator-qt5 qcollectiongenerator
	  DOC "The path of the Qt Assistant executable"
	)
	add_executable(Qt5::qcollectiongenerator IMPORTED)
	set_target_properties(Qt5::qcollectiongenerator PROPERTIES
	  IMPORTED_LOCATION ${Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE}
	)
elseif(NOT Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE)
	find_package(Qt5Help REQUIRED)
	set(Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE Qt5::qcollectiongenerator)
endif()


set(Mapper_HELP_NAMESPACE "openorienteering.mapper-${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}.help"
  CACHE INTERNAL "The namespace where the current version's help will be located.")

set(Mapper_HELP_COLLECTION "Mapper ${Mapper_VERSION_DISPLAY} Manual.qhc")
set(Mapper_COMPRESSED_HELP "Mapper ${Mapper_VERSION_DISPLAY} Manual.qch")
configure_file(Manual.qhcp.in Manual.qhcp @ONLY)

find_program(DOXYGEN_EXECUTABLE
  NAMES doxygen
  DOC "The path of the doxygen executable"
)
if(DOXYGEN_EXECUTABLE MATCHES NOTFOUND)
	message(FATAL_ERROR "doxygen executable not found.")
endif()
mark_as_advanced(DOXYGEN_EXECUTABLE)

configure_file(preprocess-markdown.cmake.in preprocess-markdown.cmake @ONLY)
add_custom_target(Mapper-manual-pages
  COMMAND "${CMAKE_COMMAND}" -P preprocess-markdown.cmake
  BYPRODUCTS preprocess-markdown.stamp
)

if(NOT MANUAL_SECTIONS)
	if(ANDROID)
		set(MANUAL_SECTIONS_DEFAULT ANDROID)
	elseif(APPLE)
		set(MANUAL_SECTIONS_DEFAULT MACOS)
	elseif(UNIX)
		set(MANUAL_SECTIONS_DEFAULT LINUX)
	elseif(WIN32)
		set(MANUAL_SECTIONS_DEFAULT WINDOWS)
	else()
		set(MANUAL_SECTIONS_DEFAULT OTHER)
	endif()
	set(MANUAL_SECTIONS "${MANUAL_SECTIONS_DEFAULT}" CACHE STRING
	  "Conditional manual sections to be enabled")
endif()
message(STATUS "Conditional manual sections: ${MANUAL_SECTIONS}")

configure_file(Doxyfile.in Doxyfile @ONLY)

configure_file(postprocess-qhp.cmake.in postprocess-qhp.cmake @ONLY)
add_custom_command( 
  OUTPUT    "html/index.qhp"
  COMMAND   "${CMAKE_COMMAND}" -E remove_directory html
  COMMAND   "${DOXYGEN_EXECUTABLE}" Doxyfile
  COMMAND   "${CMAKE_COMMAND}" -P postprocess-qhp.cmake
  MAIN_DEPENDENCY preprocess-markdown.stamp
  DEPENDS   Mapper-manual-pages
            "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
            "${CMAKE_CURRENT_BINARY_DIR}/postprocess-qhp.cmake"
            "${DOXYGEN_EXECUTABLE}"
            header.html
            footer.html
            style.css
  COMMENT   "Running doxygen on the manual"
)

if(Mapper_MANUAL_QTHELP)
	# Reproducible builds need a modifications to the help collection file.
	set(source_date_commands )
	find_program(SQLITE3_EXECUTABLE NAMES sqlite3
	  DOC "The path of the sqlite3 executable"
	)
	if (DEFINED ENV{SOURCE_DATE_EPOCH} AND SQLITE3_EXECUTABLE)
		file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/timestamp.sql"
		  "update 'SettingsTable' set Value='$ENV{SOURCE_DATE_EPOCH}' where Key='CreationTime';\n"
		  "delete from 'SettingsTable' where Key='LastRegisterTime';\n"
		)
		set(source_date_commands
	      COMMAND ${SQLITE3_EXECUTABLE} "${Mapper_HELP_COLLECTION}" < "timestamp.sql"
		)
	elseif(DEFINED ENV{SOURCE_DATE_EPOCH})
		message(FATAL_ERROR
		  "By setting SOURCE_DATE_EPOCH, a reproducible build was requested. "
		  "sqlite3 is needed for reproducible builds but cannot be found."
		)
	endif()
	set(MAPPER_MANUAL_TARGET "${Mapper_HELP_COLLECTION}")
	add_custom_command( 
	  OUTPUT    "${Mapper_HELP_COLLECTION}"
	            "${Mapper_COMPRESSED_HELP}"
	  COMMAND   "${Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE}" Manual.qhcp
	            -o "${Mapper_HELP_COLLECTION}"
	  ${source_date_commands}
	  MAIN_DEPENDENCY "html/index.qhp"
	  DEPENDS   "${CMAKE_CURRENT_BINARY_DIR}/Manual.qhcp"
	  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	  COMMENT   "Running qcollectiongenerator on the manual"
	)
	install(
	  FILES "${CMAKE_CURRENT_BINARY_DIR}/${Mapper_HELP_COLLECTION}" 
	        "${CMAKE_CURRENT_BINARY_DIR}/${Mapper_COMPRESSED_HELP}" 
	  DESTINATION "${MAPPER_ABOUT_DESTINATION}"
	)
else()
	set(MAPPER_MANUAL_TARGET "html/index.qhp")
	file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" \n
	  "FILE_PATTERNS          = android* toolbar*" \n
	  "USE_MDFILE_AS_MAINPAGE = android-index.md" \n
	)
	install(CODE "
		file(READ \"${CMAKE_CURRENT_BINARY_DIR}/html/index.qhp\" index_qhp)
		string(REGEX MATCHALL \"<file>[^<]*</file>\" matches \${index_qhp})
		foreach(match \${matches})
			string(REGEX REPLACE \".*<file>\"  \"\" match \"\${match}\")
			string(REGEX REPLACE \"</file>.*\" \"\" match \"\${match}\")
			file(INSTALL
			  DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${MAPPER_ABOUT_DESTINATION}/manual\"
			  FILES \"${CMAKE_CURRENT_BINARY_DIR}/html/\${match}\"
			)
		endforeach()
")
endif()

add_custom_target(Mapper-manual ALL
  DEPENDS   "${MAPPER_MANUAL_TARGET}"
  # Sources to be listed in Qt Creator
  SOURCES
    header.html
    footer.html
    style.css
)
