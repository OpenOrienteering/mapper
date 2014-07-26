#
#    Copyright 2012, 2013, 2014 Kai Pastor
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
 
message(STATUS "Configuring ${PROJECT_NAME} packaging")

if(APPLE)
	# Set some Qt4 variables to reasonable values for Qt5, too.
	# (needed for DeployQt5.cmake)
	find_package(Qt5Core REQUIRED QUIET)
	if(NOT QT_LIBRARY_DIR)
		get_target_property(_qt5core_lib Qt5::Core IMPORTED_LOCATION_RELEASE)
		get_filename_component(QT_LIBRARY_DIR ${_qt5core_lib} PATH)
	endif()
	if(NOT QT_BINARY_DIR)
		get_target_property(_qt5moc_bin Qt5::moc IMPORTED_LOCATION)
		get_filename_component(QT_BINARY_DIR ${_qt5moc_bin} PATH)
	endif()
endif()

if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
	# cf. http://www.cmake.org/cmake/help/cmake-2-8-docs.html#module:CPack
	# cf. http://www.cmake.org/Wiki/CMake:CPackPackageGenerators
	set(CPACK_PACKAGE_NAME "OpenOrienteering Mapper")
	set(CPACK_PACKAGE_VENDOR "OpenOrienteering")
	set(CPACK_PACKAGE_VERSION_MAJOR ${Mapper_VERSION_MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${Mapper_VERSION_MINOR})
	set(CPACK_PACKAGE_VERSION_PATCH ${Mapper_VERSION_PATCH})
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY 
	  "Map drawing program from OpenOrienteering")
	if(NOT CMAKE_SIZEOF_VOID_P AND MINGW)
		set(_env_lang $ENV{LANG})
		set(ENV{LANG} C)
		execute_process(
		  COMMAND ${CMAKE_C_COMPILER} -dumpmachine
		  OUTPUT_VARIABLE HOST_TRIPLET
		)
		set(ENV{LANG} ${_env_lang})
		if(${HOST_TRIPLET} MATCHES ^i686)
			set(CMAKE_SIZEOF_VOID_P 4)
		elseif(${HOST_TRIPLET} MATCHES ^x86_64)
			set(CMAKE_SIZEOF_VOID_P 8)
		endif()
	endif()
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(_system_name "${CMAKE_SYSTEM_NAME}-x86")
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_system_name "${CMAKE_SYSTEM_NAME}-x64")
	else()
		set(_system_name "${CMAKE_SYSTEM_NAME}-unknown")
	endif()
	set(CPACK_PACKAGE_FILE_NAME 
	  "openorienteering-mapper_${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${_system_name}")
	set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
	set(CPACK_STRIP_FILES "TRUE")
	
	set(CPACK_SOURCE_PACKAGE_FILE_NAME
	  "openorienteering-mapper_${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-src")
	set(CPACK_SOURCE_IGNORE_FILES 
	  "${PROJECT_BINARY_DIR}"
	  "/docs/"
	  "/[.]git/"
	  "/3rd-party/clipper/download/"
	  "/3rd-party/proj/download/"
	  "/3rd-party/qt5/download/"
	  ${CPACK_SOURCE_IGNORE_FILES})
	
	set(MAPPER_MACOS_SUBDIR "")

	if(WIN32)
		# Packaging as ZIP archive
		set(CPACK_GENERATOR_DEFAULT "ZIP")
		#set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
		set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenOrienteering Mapper ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
		set(CPACK_PACKAGE_EXECUTABLES "Mapper" "OpenOrienteering Mapper ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

		find_program(MAKENSIS_EXECUTABLE "makensis")
		if(MAKENSIS_EXECUTABLE)
			list(APPEND CPACK_GENERATOR_DEFAULT "NSIS")
			# The title displayed at the top of the installer
			set(CPACK_NSIS_PACKAGE_NAME "OpenOrienteering")
			# The display name string that appears in the Windows Add/Remove Program control panel
			set(CPACK_NSIS_DISPLAY_NAME "OpenOrienteering Mapper ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
			# NSIS start menu links will point to executables in this directory
			set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
			# A path to the executable that contains the uninstaller icon.
			set(CPACK_NSIS_INSTALLED_ICON_NAME Mapper.exe)
			# URL to a web site providing more information about your application.
			set(CPACK_NSIS_URL_INFO_ABOUT "http://oorienteering.sourceforge.net/?cat=3")
			# Extra NSIS commands that will be added to the install/uninstall sections.
			set(_nsis_extra_inst 
			  ReadEnvStr $1 ComSpec\n
			  ExecWait '\\\"$1\\\" /C assoc .omap=OpenOrienteering Map' $0\n
			  ExecWait '\\\"$1\\\" /C assoc .xmap=OpenOrienteering Map' $0\n
			  ExecWait '\\\"$1\\\" /C ftype OpenOrienteering Map=\\\"$INSTDIR\\\\Mapper.exe\\\" \\\"%1\\\"' $0)
			string(REPLACE ";" " " _nsis_extra_inst "${_nsis_extra_inst}")
			set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS ${_nsis_extra_inst})
			set(_nsis_extra_uninst 
			  ReadEnvStr $1 ComSpec\n
			  ExecWait '\\\"$1\\\" /C ftype OpenOrienteering Map= ' $0\n
			  ExecWait '\\\"$1\\\" /C assoc .xmap=' $0\n
			  ExecWait '\\\"$1\\\" /C assoc .omap=' $0)
			string(REPLACE ";" " " _nsis_extra_uninst "${_nsis_extra_uninst}")
			set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${_nsis_extra_uninst})
		endif(MAKENSIS_EXECUTABLE)

	elseif(APPLE)
		set(MAPPER_MACOS_SUBDIR "/Mapper.app/Contents/MacOS")
		set(CPACK_GENERATOR_DEFAULT "DragNDrop")
		set(CPACK_PACKAGE_EXECUTABLES "Mapper" "OpenOrienteering Mapper ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
		set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/images/mapper-icon/Mapper.icns")
		set_target_properties(Mapper PROPERTIES
		  MACOSX_BUNDLE_INFO_STRING          "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}"
		  MACOSX_BUNDLE_ICON_FILE            "Mapper.icns"
		  MACOSX_BUNDLE_GUI_IDENTIFIER       "org.openorienteering.Mapper"
		  MACOSX_BUNDLE_LONG_VERSION_STRING  "Mapper ${Mapper_VERSION_DISPLAY} for Mac OS X"
		  MACOSX_BUNDLE_BUNDLE_NAME          "Mapper" # less than 16 characters long
		  MACOSX_BUNDLE_SHORT_VERSION_STRING "${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}"
		  MACOSX_BUNDLE_BUNDLE_VERSION       "${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}"
		  MACOSX_BUNDLE_COPYRIGHT            "(C) 2012, 2013 OpenOrienteering developers"

		)
		install(FILES "${CPACK_PACKAGE_ICON}"
		  DESTINATION "${MAPPER_DATA_DESTINATION}")

	elseif(UNIX AND EXISTS /usr/bin/dpkg AND EXISTS /usr/bin/lsb_release)
		# Packaging on Debian or similar
		set(CPACK_GENERATOR_DEFAULT "DEB")
		execute_process(
		  COMMAND /usr/bin/lsb_release -sc 
		  OUTPUT_VARIABLE CPACK_LSB_RELEASE 
		  OUTPUT_STRIP_TRAILING_WHITESPACE)
		string(REPLACE 
		  "Linux-x86" 
		  "${CPACK_LSB_RELEASE}_i386" 
		  CPACK_PACKAGE_FILE_NAME
		  ${CPACK_PACKAGE_FILE_NAME})
		string(REPLACE 
		  "Linux-x64" 
		  "${CPACK_LSB_RELEASE}_amd64" 
		  CPACK_PACKAGE_FILE_NAME
		  ${CPACK_PACKAGE_FILE_NAME})
		set(CPACK_DEBIAN_PACKAGE_MAINTAINER
		   "Kai Pastor <dg0yt@darc.de>")
		set(CPACK_DEBIAN_SECTION "graphics")
		set(CPACK_DEBIAN_PACKAGE_HOMEPAGE 
		  "http://oorienteering.sourceforge.net/?cat=3")
		set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
		
		unset(FAKEROOT_EXECUTABLE CACHE)
		find_program(FAKEROOT_EXECUTABLE fakeroot)
		if(NOT FAKEROOT_EXECUTABLE)
			install(CODE "MESSAGE(WARNING
			  \"'fakeroot' not found. To build a DEB package with proper file \"
			  \"ownership, fakeroot must be installed.\")")
		endif(NOT FAKEROOT_EXECUTABLE)
		mark_as_advanced(FAKEROOT_EXECUTABLE)
		
		message(WARNING 
		  "The CPackDeb generator for creating deb packages has some issues. Run\n"
		  "   make deb\n"
		  "instead of running make package directly." )
		add_custom_target(deb
		  COMMENT "Building a DEB package"
		  COMMAND sh -c 'if [ -d \"${CPACK_PACKAGE_FILE_NAME}\" ]; then rm -R \"${CPACK_PACKAGE_FILE_NAME}\" ";" fi'
		  COMMAND sh -c 'mkdir \"${CPACK_PACKAGE_FILE_NAME}\" && chmod 0755 \"${CPACK_PACKAGE_FILE_NAME}\"'
		  COMMAND sh -c '
		    umask 022 && 
		    "\$(MAKE)" package &&
		    dpkg-deb -R \"${CPACK_PACKAGE_FILE_NAME}.deb\" \"${CPACK_PACKAGE_FILE_NAME}\" &&
		    ldconfig -n \"${CPACK_PACKAGE_FILE_NAME}${MAPPER_LIBRARY_DESTINATION}/lib\" &&
		    fakeroot dpkg-deb -b \"${CPACK_PACKAGE_FILE_NAME}\"'
		  COMMAND rm -R "./${CPACK_PACKAGE_FILE_NAME}"
		  WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
	
	endif()

	set(CPACK_GENERATOR "${CPACK_GENERATOR_DEFAULT}"
	  CACHE STRING "The binary package generators (ZIP;DEB;DragNDrop;NSIS)")
	set(CPACK_SOURCE_GENERATOR "OFF"
	  CACHE STRING "The source package generators (TGZ;ZIP)")
	mark_as_advanced(CPACK_GENERATOR CPACK_SOURCE_GENERATOR)

	include(CPack)
else()
	message(STATUS "CPack not found, target 'package' will not be available")
endif(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")


if(WIN32)
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
endif()
include(InstallRequiredSystemLibraries)


# Cleanup obsolete cache items
unset(MAPPER_LIBS CACHE)
unset(MAPPER_QT_PLUGINS CACHE)
unset(MAPPER_QT_TRANSLATIONS CACHE)

unset(MAPPER_LIB_HINTS)
unset(MAPPER_LIBS)

if(TOOLCHAIN_SHARED_LIBS)
	list(APPEND MAPPER_LIBS ${TOOLCHAIN_SHARED_LIBS})
elseif(MINGW)
	list(APPEND MAPPER_LIBS libgcc_s_dw2-1 mingwm10)
	list(APPEND MAPPER_LIBS libgcc_s_sjlj-1 libwinpthread-1 libstdc++-6)
endif()

if(Mapper_PACKAGE_PROJ)
	install(
	  DIRECTORY "${PROJ_BINARY_DIR}/../share/proj"
	  DESTINATION "${MAPPER_DATA_DESTINATION}")
	if(WIN32)
		set(PROJ_LIB_SUFFIX -0)
	else()
		set(PROJ_LIB_SUFFIX "")
	endif()
	list(APPEND MAPPER_LIBS      proj${PROJ_LIB_SUFFIX})
	list(APPEND MAPPER_LIB_HINTS ${PROJ_BINARY_DIR})
endif()

unset(MAPPER_QT_PLUGINS)
unset(MAPPER_QT_TRANSLATIONS)
if(Mapper_PACKAGE_QT)
	set(QT_LIB_SUFFIX "")
	list(APPEND MAPPER_LIB_HINTS ${QT_BINARY_DIR})
	if(WIN32)
		install(
		  FILES "${PROJECT_BINARY_DIR}/3rd-party/qt5/qt.conf"
		  DESTINATION "${MAPPER_RUNTIME_DESTINATION}")
	endif()
	list(APPEND MAPPER_LIBS 
	  Qt5Core Qt5Gui Qt5Widgets
	  Qt5PrintSupport Qt5Network Qt5Xml)
	set(MAPPER_QT_PLUGINS
	  accessible/libqtaccessiblequick
	  accessible/libqtaccessiblewidgets
	  designer/libqquickwidget
	  generic/libqevdevkeyboardplugin
	  generic/libqevdevmouseplugin
	  generic/libqevdevtabletplugin
	  generic/libqevdevtouchplugin
	  imageformats/qgif
	  imageformats/libqicns
	  imageformats/qico
	  imageformats/qjp2
	  imageformats/qjpeg
	  imageformats/qtiff
	  imageformats/qwebp
	  platforminputcontexts/libcomposeplatforminputcontextplugin
	  platforminputcontexts/libibusplatforminputcontextplugin
	  platforms/qcocoa
	  platforms/qminimal
	  platforms/libqoffscreen
	  platforms/qwindowsplatforms/qxcb
	  position/libqtposition_positionpoll
	  printsupport/cocoaprintersupport
	  printsupport/libcupsprintersupport
	  printsupport/windowsprintersupport
	  sensorgestures/libqtsensorgestures_plugin
	  sensorgestures/libqtsensorgestures_shakeplugin
	  sensors/libqtsensors_dummy
	  sensors/libqtsensors_generic
	  sensors/libqtsensors_linuxsys
	  CACHE INTERNAL "The plugins which need to be deployed"
	)
	# Cf. Qt5's qt_de.ts for dependencies - qt_de.ts would not load without them.
	list(APPEND MAPPER_QT_TRANSLATIONS qt qtbase qtscript qtquick1 qtmultimedia qtxmlpatterns)
endif()

unset(FIXUP_ASSISTANT)
if(Mapper_PACKAGE_ASSISTANT)
	if(NOT Qt5Help_ASSISTANT_EXECUTABLE)
		message(FATAL_ERROR "Qt5Help_ASSISTANT_EXECUTABLE: not found, "
		  "but required by option Mapper_PACKAGE_ASSISTANT="
		  ${Mapper_PACKAGE_ASSISTANT})
	endif()
	message(STATUS "Qt Assistant - found")
	if(WIN32 OR APPLE)
		install(
		  PROGRAMS ${Qt5Help_ASSISTANT_EXECUTABLE}
		  DESTINATION "${MAPPER_RUNTIME_DESTINATION}${MAPPER_MACOS_SUBDIR}")
	else()
		install(
		  PROGRAMS ${Qt5Help_ASSISTANT_EXECUTABLE}
		  DESTINATION "${MAPPER_LIBRARY_DESTINATION}/bin")
		install(
		  FILES "${PROJECT_BINARY_DIR}/3rd-party/qt5/qt.conf"
		  DESTINATION "${MAPPER_LIBRARY_DESTINATION}/bin")
	endif()
	if(NOT APPLE)
		list(APPEND MAPPER_LIBS 
		  Qt5Help Qt5CLucene Qt5Sql
		)
	endif()
	if(APPLE)
		set(FIXUP_ASSISTANT "Mapper.app/Contents/MacOS/Assistant")
	endif()
	list(APPEND MAPPER_QT_TRANSLATIONS qt_help assistant)
endif(Mapper_PACKAGE_ASSISTANT)

if(CMAKE_CROSSCOMPILING AND MINGW)
	set(_env_lang $ENV{LC_ALL})
	set(ENV{LC_ALL} C)
	execute_process(
	  COMMAND ${CMAKE_C_COMPILER} --print-search-dirs
	  OUTPUT_VARIABLE MINGW_SEARCH_DIRS
	)
	set(ENV{LC_ALL} ${_env_lang})
	string(REGEX REPLACE ".*libraries: ?=?([^\n]*).*" \\1 MINGW_SEARCH_DIRS "${MINGW_SEARCH_DIRS}")
	string(REPLACE \; \\\; MINGW_SEARCH_DIRS "${MINGW_SEARCH_DIRS}")
	string(REPLACE : \; MINGW_SEARCH_DIRS "${MINGW_SEARCH_DIRS}")
	list(APPEND MAPPER_LIB_HINTS ${MINGW_SEARCH_DIRS})
endif()

# Install all libs
foreach(_mapper_lib ${MAPPER_LIBS})
	set(_mapper_lib_id "MAPPER_${_mapper_lib}_LIB")
	find_library(${_mapper_lib_id} ${_mapper_lib}
	  HINTS ${MAPPER_LIB_HINTS}
	  PATH_SUFFIXES "../bin" ${TOOLCHAIN_PATH_SUFFIXES}
	  NO_CMAKE_FIND_ROOT_PATH)
	get_filename_component(_mapper_lib_ext "${${_mapper_lib_id}}" EXT)
	if(_mapper_lib_ext STREQUAL ${CMAKE_SHARED_LIBRARY_SUFFIX})
		message("   ${_mapper_lib} shared library - found")
		list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${${_mapper_lib_id}}")
		if(IS_SYMLINK "${${_mapper_lib_id}}")
			get_filename_component(_mapper_lib_real "${${_mapper_lib_id}}" REALPATH)
			list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${_mapper_lib_real}")
		endif()
	else()
		message("   ${_mapper_lib} shared library - not found")
	endif()
endforeach(_mapper_lib)
if(WIN32)
	install(
	  FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} 
	  DESTINATION "${MAPPER_LIBRARY_DESTINATION}")
elseif(UNIX AND NOT APPLE) # Libs on OSX will be handled by fixup_bundle.
	install(
	  FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} 
	  DESTINATION "${MAPPER_LIBRARY_DESTINATION}/lib")
endif()

# Install all plugins
unset(MAPPER_QT_PLUGINS_FOUND)
foreach(_qt_plugin ${MAPPER_QT_PLUGINS})
	get_filename_component(_qt_plugin_name "${_qt_plugin}" NAME)
	get_filename_component(_qt_plugin_dir "${_qt_plugin}" PATH)
	set(_qt_plugin_id "QT_${_qt_plugin_dir}-${_qt_plugin_name}_PLUGIN")
	find_library(${_qt_plugin_id} ${_qt_plugin_name}
	  HINTS ${QT_PLUGINS_DIR}
	  PATH_SUFFIXES ${_qt_plugin_dir} plugins/${_qt_plugin_dir}
	  NO_CMAKE_FIND_ROOT_PATH)
	if(NOT ${${_qt_plugin_id}} MATCHES NOTFOUND)
		message("   ${_qt_plugin} plugin library - found")
		list(APPEND MAPPER_QT_PLUGINS_FOUND "${${_qt_plugin_id}}")
		if(NOT APPLE)
			install(
			  FILES "${${_qt_plugin_id}}" 
			  DESTINATION "${MAPPER_LIBRARY_DESTINATION}/plugins/${_qt_plugin_dir}")
		endif()
	else()
		message("   ${_qt_plugin} plugin library - not found")
	endif()
endforeach(_qt_plugin)

if(APPLE)
	include(${PROJECT_SOURCE_DIR}/cmake/DeployQt5.cmake)
	install_qt5_executable(Mapper.app
	  "${MAPPER_QT_PLUGINS_FOUND}"
	  ${FIXUP_ASSISTANT})
elseif(UNIX)
	find_program(LDCONFIG_COMMAND NAMES ldconfig)
	if(${LDCONFIG_COMMAND} MATCHES NOTFOUND)
		message(FATAL_ERROR "Could not find ldconfig command.")
	endif()
	# Add required symlinks.
	install(CODE "execute_process(COMMAND \"${LDCONFIG_COMMAND}\" -n \"\$ENV{DESTDIR}${MAPPER_LIBRARY_DESTINATION}/lib\")")
	# Create DEBIAN directory to make dpkg-shlibdeps happy.
	install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}/DEBIAN\")") 
endif()

# Install Qt translations
set(Mapper_PACKAGE_QT_ALL_TRANSLATIONS TRUE)
if(Mapper_PACKAGE_QT AND Mapper_PACKAGE_QT_ALL_TRANSLATIONS)
	# Simply install Qt translations for all languages
	foreach(_qt_component ${MAPPER_QT_TRANSLATIONS})
		install(
		  DIRECTORY "${QT_TRANSLATIONS_DIR}/"
		  DESTINATION "${MAPPER_DATA_DESTINATION}/translations"
		  FILES_MATCHING PATTERN "${_qt_component}_*.qm")
	endforeach()
elseif(Mapper_PACKAGE_QT AND NOT Mapper_PACKAGE_QT_ALL_TRANSLATIONS)
	# Install Qt translations only for existing Mapper translations
	message(STATUS "Checking for Qt translations")
	unset(_install_translations)
	foreach(_mapper_trans ${Mapper_TRANS})
		unset(_found_translations)
		
		get_filename_component(_qt_translation ${_mapper_trans} NAME_WE)
		string(REPLACE OpenOrienteering_ "" _mapper_lang ${_qt_translation})
		
		foreach(_qt_component ${MAPPER_QT_TRANSLATIONS})
			set(_qt_translation "${_qt_component}_${_mapper_lang}")
			set(_qt_translation_path "${QT_TRANSLATIONS_DIR}/${_qt_translation}.qm")
			if(EXISTS "${_qt_translation_path}")
				list(APPEND _found_translations ${_qt_translation})
				list(APPEND _install_translations "${_qt_translation_path}")
			endif()
		endforeach()
		if (_found_translations)
			message(STATUS "Checking for Qt translations - ${_mapper_lang} found: ${_found_translations}")
		else()
			message(STATUS "Checking for Qt translations - ${_mapper_lang} not found")
		endif()
	endforeach(_mapper_trans)
	install(
	  FILES ${_install_translations} 
	  DESTINATION "${MAPPER_DATA_DESTINATION}/translations")
endif()


if(UNIX AND NOT APPLE)
	install(
	  FILES "${PROJECT_SOURCE_DIR}/docs/man/Mapper.1"
	  DESTINATION "share/man/man1")
	install(
	  FILES "debian/Mapper.desktop"
	  DESTINATION "share/applications")
	# Cf. http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html#directory_layout
	foreach(_size 16 24 32 48 96 128 256 512)
		install(
		  FILES "${PROJECT_SOURCE_DIR}/images/mapper-icon/Mapper-${_size}.png"
		  DESTINATION "share/icons/hicolor/${_size}x${_size}/apps"
		  RENAME Mapper.png
		)
	endforeach()
endif()

message(STATUS "Configuring ${PROJECT_NAME} packaging - done")

if(Mapper_BUILD_QT AND Mapper_PACKAGE_LINGUIST)
	include(ExternalProject)
	set(CMAKE_TOOLCHAIN_ARG "")
	if(CMAKE_TOOLCHAIN_FILE)
		set(CMAKE_TOOLCHAIN_ARG "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
	endif()
	ExternalProject_Add("Linguist package"
	  DEPENDS Qt5
	  SOURCE_DIR ${PROJECT_SOURCE_DIR}/packaging/linguist
	  UPDATE_COMMAND ${CMAKE_COMMAND} -E remove <BINARY_DIR>/CMakeCache.txt
	  CMAKE_ARGS
	    ${CMAKE_TOOLCHAIN_ARG}
	    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
	    -DQT5_DIR=${QT5_DIR}
		-DQT_TRANSLATIONS_DIR=${QT_TRANSLATIONS_DIR}
		-DQT_CONF=${PROJECT_BINARY_DIR}/3rd-party/qt5/qt.conf
	  BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target package
	  INSTALL_COMMAND ""
	)
	ExternalProject_Get_Property("Linguist package" BINARY_DIR)
	# Copy the linguist package to the build tree root during install/packaging
	install(CODE
	  "execute_process(COMMAND \${CMAKE_COMMAND} --build \"${BINARY_DIR}\" --target \"package-install\" -- \"DESTDIR=${PROJECT_BINARY_DIR}\")"
	)
endif()
