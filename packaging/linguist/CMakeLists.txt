#
#    Copyright 2013 Kai Pastor
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
 
project("Qt Linguist")

cmake_minimum_required(VERSION 2.8)

message(STATUS "Configuring ${PROJECT_NAME} packaging")


# Find Linguist

find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Core REQUIRED)
find_program(QT_QTLINGUIST_EXECUTABLE NAMES linguist${CMAKE_EXECUTABLE_SUFFIX} Linguist${CMAKE_EXECUTABLE_SUFFIX}
  DOC "The path of the Qt Linguist executable"
  HINTS ${QT5_DIR}
  PATH_SUFFIXES bin
  NO_DEFAULT_PATH)
mark_as_advanced(QT_QTLINGUIST_EXECUTABLE)

if(QT_QTLINGUIST_EXECUTABLE MATCHES NOTFOUND)
	message(FATAL_ERROR "Qt Linguist executable not found.")
endif()


# Configure CPack

set(CPACK_PACKAGE_NAME "Qt Linguist")
set(CPACK_PACKAGE_VENDOR "Qt")
set(CPACK_PACKAGE_VERSION_MAJOR ${Qt5Core_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${Qt5Core_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${Qt5Core_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY 
  "Translation tool from Qt")
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(_system_name "${CMAKE_SYSTEM_NAME}-x86")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_system_name "${CMAKE_SYSTEM_NAME}-x64")
else()
	set(_system_name "${CMAKE_SYSTEM_NAME}-unknown")
endif()
set(CPACK_PACKAGE_FILE_NAME 
  "qt-linguist_${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${_system_name}")
#	set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
#	set(CPACK_STRIP_FILES "TRUE")

if(WIN32)
	set(RUNTIME_DESTINATION .)
	set(LIBRARY_DESTINATION .)
	set(DATA_DESTINATION    .)
	set(ABOUT_DESTINATION   "doc")
	set(PACKAGE_FILE_NAME   ${CPACK_PACKAGE_FILE_NAME}.zip)
	set(CPACK_GENERATOR     "ZIP")
elseif(APPLE)
	set(RUNTIME_DESTINATION .)
	set(LIBRARY_DESTINATION .)
	set(DATA_DESTINATION    "${PROJECT_NAME}.app/Contents/Resources")
	set(ABOUT_DESTINATION   "${PROJECT_NAME}.app/Contents/Resources")
	set(PACKAGE_FILE_NAME ${CPACK_PACKAGE_FILE_NAME}.dmg)
	set(CPACK_GENERATOR     "DragNDrop")
else()
	set(RUNTIME_DESTINATION .)
	set(LIBRARY_DESTINATION .)
	set(DATA_DESTINATION    .)
	set(ABOUT_DESTINATION   "doc")
	set(PACKAGE_FILE_NAME   ${CPACK_PACKAGE_FILE_NAME}.zip)
	set(CPACK_GENERATOR     "ZIP")
endif()

include(CPack)


# Copy the created package to another location

set(MAKE_DESTDIR_VAR "\$(DESTDIR)")
add_custom_target(package-install
  COMMAND ${CMAKE_COMMAND} -E
    copy ${PROJECT_BINARY_DIR}/${PACKAGE_FILE_NAME} "${MAKE_DESTDIR_VAR}/"
)


# Define package contents (by install commands)

install(
  PROGRAMS ${QT_QTLINGUIST_EXECUTABLE}
  DESTINATION ${RUNTIME_DESTINATION}
)

if(WIN32)
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
endif()
include(InstallRequiredSystemLibraries)

unset(LINGUIST_LIBS)
if(TOOLCHAIN_SHARED_LIBS)
	list(APPEND LINGUIST_LIBS ${TOOLCHAIN_SHARED_LIBS})
elseif(MINGW)
	list(APPEND LINGUIST_LIBS libgcc_s_dw2-1 mingwm10)
	list(APPEND LINGUIST_LIBS libgcc_s_sjlj-1 libwinpthread-1 libstdc++-6)
endif()

set(QT_LIB_SUFFIX "")
list(APPEND LINGUIST_LIB_HINTS ${QT_BINARY_DIR})
if(FALSE AND WIN32)
	install(
	  FILES "${PROJECT_BINARY_DIR}/3rd-party/qt5/qt.conf"
	  DESTINATION "${LINGUIST_RUNTIME_DESTINATION}")
endif()
if(NOT APPLE)
	list(APPEND LINGUIST_LIBS Qt5Core Qt5Gui Qt5Widgets Qt5PrintSupport Qt5Xml)
else()
	set(FIXUP_LINGUIST "Linguist.app/Contents/MacOS/Linguist")
endif()
set(LINGUIST_QT_PLUGINS
#  accessible/qtaccessiblewidgets
#  iconengines/qsvgicon
#  imageformats/qgif imageformats/qico imageformats/qjpeg imageformats/qmng
#  imageformats/qsvg imageformats/qtga imageformats/qtiff imageformats/qwbmp
  platforms/qminimal platforms/qwindows platforms/qcocoa platforms/qxcb
  printsupport/windowsprintersupport printsupport/cocoaprintersupport printsupport/cupsprintersupport
)
# Cf. Qt5's qt_de.ts for dependencies - qt_de.ts would not load without them.
set(LINGUIST_QT_TRANSLATIONS linguist qt qtbase qtscript qtquick1 qtmultimedia qtxmlpatterns)

# Install all libs
foreach(_linguist_lib ${LINGUIST_LIBS})
	set(_linguist_lib_id "LINGUIST_${_linguist_lib}_LIB")
	find_library(${_linguist_lib_id} ${_linguist_lib}
	  HINTS ${LINGUIST_LIB_HINTS}
	  PATH_SUFFIXES "../bin" ${TOOLCHAIN_PATH_SUFFIXES}
	  NO_CMAKE_FIND_ROOT_PATH)
	get_filename_component(_linguist_lib_ext "${${_linguist_lib_id}}" EXT)
	if(_linguist_lib_ext STREQUAL ${CMAKE_SHARED_LIBRARY_SUFFIX})
		message("   ${_linguist_lib} shared library - found")
		list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${${_linguist_lib_id}}")
		if(IS_SYMLINK "${${_linguist_lib_id}}")
			get_filename_component(_linguist_lib_real "${${_linguist_lib_id}}" REALPATH)
			list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${_linguist_lib_real}")
		endif()
	else()
		message("   ${_linguist_lib} shared library - not found")
	endif()
endforeach(_linguist_lib)
if(WIN32)
	install(
	  FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} 
	  DESTINATION "${LIBRARY_DESTINATION}")
elseif(UNIX AND NOT APPLE) # Libs on OSX will be handled by fixup_bundle.
	install(
	  FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} 
	  DESTINATION "${LIBRARY_DESTINATION}/lib")
endif()

# Install all plugins
unset(QT_PLUGINS_FOUND)
foreach(_qt_plugin ${LINGUIST_QT_PLUGINS})
	get_filename_component(_qt_plugin_name "${_qt_plugin}" NAME)
	get_filename_component(_qt_plugin_dir "${_qt_plugin}" PATH)
	set(_qt_plugin_id "QT_${_qt_plugin_dir}-${_qt_plugin_name}_PLUGIN")
	find_library(${_qt_plugin_id} ${_qt_plugin_name}
	  HINTS ${QT_PLUGINS_DIR}
	  PATH_SUFFIXES ${_qt_plugin_dir} plugins/${_qt_plugin_dir}
	  NO_CMAKE_FIND_ROOT_PATH)
	if(NOT ${${_qt_plugin_id}} MATCHES NOTFOUND)
		message("   ${_qt_plugin} plugin library - found")
		list(APPEND QT_PLUGINS_FOUND "${${_qt_plugin_id}}")
		if(NOT APPLE)
			install(
			  FILES "${${_qt_plugin_id}}" 
			  DESTINATION "${LIBRARY_DESTINATION}/plugins/${_qt_plugin_dir}")
		endif()
	else()
		message("   ${_qt_plugin} plugin library - not found")
	endif()
endforeach(_qt_plugin)

# Install all Qt translations
foreach(_qt_component ${LINGUIST_QT_TRANSLATIONS})
	install(
	  DIRECTORY "${QT_TRANSLATIONS_DIR}/"
	  DESTINATION "${DATA_DESTINATION}/translations"
	  FILES_MATCHING PATTERN "${_qt_component}_*.qm")
endforeach()

if(WIN32)
	install(
	  FILES ${QT_CONF}
	  DESTINATION ${RUNTIME_DESTINATION})
endif()

message(STATUS "Configuring ${PROJECT_NAME} packaging - done")
