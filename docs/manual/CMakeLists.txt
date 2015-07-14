#
#    Copyright 2012, 2013, 2014 Thomas Schöps
#    Copyright 2012-2015 Kai Pastor
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
	project(Mapper_Manual NONE)
	cmake_minimum_required(VERSION 2.8.8)
else()
	message(STATUS "Configuring ${PROJECT_NAME} user manual")
endif()

if(NOT ANDROID) # ---------------------------------------------------

if (CMAKE_CROSSCOMPILING)
	find_program(Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE
	  NAMES qcollectiongenerator-qt5 qcollectiongenerator
	  DOC "The path of the Qt Assistant executable"
	)
	add_executable(Qt5::qcollectiongenerator IMPORTED)
	set_target_properties(Qt5::qcollectiongenerator PROPERTIES
		IMPORTED_LOCATION ${Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE}
	)
endif()

enable_language(CXX)
find_package(Qt5Help REQUIRED)

# Not exported from Qt5
if (NOT TARGET Qt5::qcollectiongenerator AND TARGET Qt5::moc)
	add_executable(Qt5::qcollectiongenerator IMPORTED)
	get_target_property(imported_location Qt5::moc IMPORTED_LOCATION)
	get_filename_component(imported_location ${imported_location} PATH)
	set(imported_location ${imported_location}/qcollectiongenerator)
	if(NOT EXISTS "${imported_location}")
		message(FATAL_ERROR "qcollectiongenerator executable does not exist: "
		                    "${imported_location}")
	endif()
	set_target_properties(Qt5::qcollectiongenerator PROPERTIES
		IMPORTED_LOCATION ${imported_location}
	)
endif()
set(Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE Qt5::qcollectiongenerator)

find_program(Qt5Help_ASSISTANT_EXECUTABLE NAMES assistant${CMAKE_EXECUTABLE_SUFFIX} Assistant${CMAKE_EXECUTABLE_SUFFIX}
  DOC "The path of the Qt Assistant executable"
  HINTS ${QT5_DIR}
  PATH_SUFFIXES bin
  NO_DEFAULT_PATH)
mark_as_advanced(Qt5Help_ASSISTANT_EXECUTABLE)

endif(NOT ANDROID) # ---------------------------------------------------

set(Mapper_HELP_NAMESPACE "openorienteering.mapper-${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}.help"
  CACHE INTERNAL "The namespace where the current version's help will be located.")

set(Mapper_HELP_COLLECTION "Mapper ${Mapper_VERSION_DISPLAY} Manual.qhc")
set(Mapper_COMPRESSED_HELP "Mapper ${Mapper_VERSION_DISPLAY} Manual.qch")
configure_file(Manual.qhcp.in Manual.qhcp @ONLY)

find_program(DOXYGEN_EXECUTABLE NAMES doxygen
	  DOC "The path of the doxygen executable")
mark_as_advanced(DOXYGEN_EXECUTABLE)

if(DOXYGEN_EXECUTABLE MATCHES NOTFOUND)
	message(WARNING "doxygen executable not found.")
else()
	configure_file(preprocess-markdown.cmake.in preprocess-markdown.cmake @ONLY)
	add_custom_target(
	  Mapper-manual-pages
	  COMMAND   ${CMAKE_COMMAND} -P preprocess-markdown.cmake
	)

	configure_file(Doxyfile.in Doxyfile @ONLY)
	
	configure_file(postprocess-qhp.cmake.in postprocess-qhp.cmake @ONLY)
	add_custom_command( 
	  OUTPUT    ${CMAKE_CURRENT_BINARY_DIR}/html/index.qhp
	  COMMAND   ${CMAKE_COMMAND}
	  ARGS      -E remove_directory html
	  COMMAND   ${DOXYGEN_EXECUTABLE}
	  ARGS      Doxyfile
	  COMMAND   ${CMAKE_COMMAND}
	  ARGS      -P postprocess-qhp.cmake
	  MAIN_DEPENDENCY preprocess-markdown.stamp
	  DEPENDS   Mapper-manual-pages
	            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	            ${CMAKE_CURRENT_BINARY_DIR}/postprocess-qhp.cmake
	            ${DOXYGEN_EXECUTABLE}
	            header.html
	            footer.html
	            style.css
	  COMMENT   "Running doxygen on the manual"
	)
    set_source_files_properties(preprocess-markdown.stamp PROPERTIES GENERATED 1)

	if(NOT ANDROID) # ---------------------------------------------------
	
	set(MAPPER_MANUAL_TARGET "${Mapper_HELP_COLLECTION}")

	add_custom_command( 
	  OUTPUT    ${Mapper_HELP_COLLECTION} ${Mapper_COMPRESSED_HELP}
	  COMMAND   ${Qt5Help_QCOLLECTIONGENERATOR_EXECUTABLE}
	  ARGS      Manual.qhcp
	            -o "${Mapper_HELP_COLLECTION}"
	  MAIN_DEPENDENCY ${CMAKE_CURRENT_BINARY_DIR}/html/index.qhp
	  DEPENDS   ${CMAKE_CURRENT_BINARY_DIR}/Manual.qhcp
	            ${CMAKE_CURRENT_BINARY_DIR}/html/index.qhp
	  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	  COMMENT   "Running qcollectiongenerator on the manual"
	)

	if(NOT CMAKE_CROSSCOMPILING)
		add_custom_target(view-Mapper-manual
		  COMMAND   "${Qt5Help_ASSISTANT_EXECUTABLE}"
		            -collectionFile "${Mapper_HELP_COLLECTION}"
		  DEPENDS   Mapper-manual
		  COMMENT   "Opening Mapper manual"
		)
	endif()
	
	install(
	  FILES "${CMAKE_CURRENT_BINARY_DIR}/${Mapper_HELP_COLLECTION}" 
	        "${CMAKE_CURRENT_BINARY_DIR}/${Mapper_COMPRESSED_HELP}" 
	  DESTINATION "${MAPPER_DATA_DESTINATION}/doc/manual")
	
	elseif(ANDROID) # ---------------------------------------------------
	
	set(MAPPER_MANUAL_TARGET "${CMAKE_CURRENT_BINARY_DIR}/html/index.qhp")
	
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
	    file(INSTALL DESTINATION /assets/doc/manual FILES \"${CMAKE_CURRENT_BINARY_DIR}/html/\${match}\")
	  endforeach()
	")
	
	endif() # ---------------------------------------------------
	
	add_custom_target(Mapper-manual ALL
	  DEPENDS   "${MAPPER_MANUAL_TARGET}"
	  # Sources to be listed in Qt Creator
	  SOURCES   header.html
	            footer.html
	            style.css
	)
	
endif()
