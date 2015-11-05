#
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

message(STATUS "Configuring ${PROJECT_NAME} API documentation")

set(API_SRCS
  extra/code_overview.h
  extra/mainpage.h
)

set(TAG_FILES 
  qtcore.tags
  qtgui.tags
  qtwidgets.tags
  qtxml.tags
  qtnetwork.tags
  qtprintsupport.tags
)

find_program(DOXYGEN_EXECUTABLE NAMES doxygen
	  DOC "The path of the doxygen executable")
mark_as_advanced(DOXYGEN_EXECUTABLE)

find_program(GIT_EXECUTABLE NAMES git
	  DOC "The path of the git executable")
mark_as_advanced(GIT_EXECUTABLE)

set(API_DOCS_REPOSITORY "https://github.com/OpenOrienteering/api-docs.git" CACHE STRING
    "The git repository where to push the api docs (only used on initial clone)")
set(API_DOCS_BRANCH "gh-pages" CACHE STRING
    "The git branch where to push the api docs")
set(API_DOCS_PATH "mapper" CACHE STRING
    "The path where to push the api docs")

if(DOXYGEN_EXECUTABLE MATCHES NOTFOUND)
	message(STATUS "doxygen executable not found.")
else()
	set(url http://doc.qt.io/qt-5/)
	foreach(file ${TAG_FILES})
		file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${file}.cmake"
			 "file(DOWNLOAD \"${url}${file}\" \"${CMAKE_CURRENT_BINARY_DIR}/${file}\")\n")
		add_custom_command(OUTPUT ${file}
		  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/${file}.cmake"
		  COMMENT "Downloading ${file} from ${url}")
	endforeach()

	configure_file(api-docs.sh.in api-docs.sh @ONLY)
	configure_file(Doxyfile.in Doxyfile @ONLY)
	configure_file(versionfilter.sh.in versionfilter.sh @ONLY)
	add_custom_target(api-docs
	  COMMAND   "${CMAKE_CURRENT_BINARY_DIR}/api-docs.sh" "${DOXYGEN_EXECUTABLE}"
	  COMMENT   "Generating API documentation"
	  DEPENDS   ${TAG_FILES}
	  SOURCES   ${API_SRCS}
	            api-docs.sh
	)
endif()

if(GIT_EXECUTABLE MATCHES NOTFOUND)
	message(STATUS "git executable not found.")
elseif(TARGET api-docs)
	add_custom_target(api-docs-repository
	  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/api-docs-repository.sh" "${API_DOCS_REPOSITORY}" "${API_DOCS_BRANCH}" "${API_DOCS_PATH}"
	  COMMENT "Preparing API docs repository"
	  SOURCES api-docs-repository.sh
	)

	add_custom_target(api-docs-commit
	  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/api-docs-commit.sh" "${API_DOCS_PATH}"
	  COMMENT "Committing changed API docs"
	  SOURCES api-docs-commit.sh
	)
	add_dependencies(api-docs-commit api-docs api-docs-repository)

	add_custom_target(api-docs-push
	  COMMAND git push
	  COMMENT "Pushing changed API docs to ${API_DOCS_REPOSITORY}"
	  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/repository"
	)
endif()
