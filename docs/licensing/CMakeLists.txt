#
#    Copyright 2014 Kai Pastor
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


message(STATUS "Configuring ${PROJECT_NAME} licensing documentation")


find_package(Qt5Core)

# Like Qt5::moc, Qt5::qdoc should come from Qt5CoreConfigExtra.cmake
if (NOT TARGET Qt5::qdoc AND TARGET Qt5::moc)
    get_target_property(_qt5_moc_path Qt5::moc IMPORTED_LOCATION)
    get_filename_component(_qt5_bin_prefix "${_qt5_moc_path}" PATH)
    add_executable(Qt5::qdoc IMPORTED)
    set_target_properties(Qt5::qdoc PROPERTIES
        IMPORTED_LOCATION "${_qt5_bin_prefix}/qdoc"
    )
endif()
set(Qt5Core_QDOC_EXECUTABLE Qt5::qdoc)


set(LICENSING_SRCS
  licensing.qdocconf
  licensing-html.qdocconf
  licensing.css
  src/licensing.qdoc
  src/qt-licensing.qdocinc
  src/trademarks.qdocinc
  src/apache-2.0.qdoc
  src/gpl-3.0.qdoc
  src/lgpl-2.1.qdoc
  src/gcc-runtime-library-exception.qdoc
)

add_custom_target(licensing-sources
  COMMENT   "This target makes Qt Creator show all sources in the project tree."
  SOURCES   licensing.pro licensing.qrc ${LICENSING_SRCS}
)

add_custom_command(
  OUTPUT    html/licensing.html
  COMMENT   "Running qdoc on licensing documentation"
  COMMAND   ${Qt5Core_QDOC_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/licensing.qdocconf"
			  -outputdir "${CMAKE_CURRENT_BINARY_DIR}/html"
  DEPENDS   ${LICENSING_SRCS}
)

add_custom_command(
  OUTPUT    licensing.qrc
  COMMAND   ${CMAKE_COMMAND} -E copy "@CMAKE_CURRENT_SOURCE_DIR@/licensing.qrc" licensing.qrc
  DEPENDS   licensing.qrc html/licensing.html
)

qt5_add_resources(LICENSING_QRC_CPP
  "${CMAKE_CURRENT_BINARY_DIR}/licensing.qrc"
  OPTIONS -compress 9 -threshold 50
)

add_library(docs-licensing
  STATIC
  ${LICENSING_QRC_CPP}
)

if (TARGET Qt5)
	add_dependencies(docs-licensing Qt5)
endif()
