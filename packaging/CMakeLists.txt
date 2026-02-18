#
#    Copyright 2012-2021, 2024-2026 Kai Pastor
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

option(Mapper_PACKAGE_QT_ALL_TRANSLATIONS "Add all Qt translations to the packages" OFF)

function(deploy_qt_translations basename)
	find_package(Qt5LinguistTools REQUIRED QUIET) # for Qt5::lconvert
	
	if(NOT Qt5Translations_DIR)
		find_package(Qt5Core REQUIRED QUIET)
		get_target_property(QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
		execute_process(
		  COMMAND "${QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS
		  OUTPUT_VARIABLE Qt5Translations_DIR
		)
		string(STRIP "${Qt5Translations_DIR}" Qt5Translations_DIR)
		if(NOT EXISTS ${Qt5Translations_DIR})
			set(Qt5Translations_DIR "Qt5Translations_DIR-NOTFOUND")
		endif()
		set(Qt5Translations_DIR "${Qt5Translations_DIR}" CACHE PATH
		  "The Qt5 translations directory"
		)
	endif()
	
	if(Mapper_PACKAGE_QT_ALL_TRANSLATIONS)
		file(GLOB basename_files RELATIVE "${Qt5Translations_DIR}" "${Qt5Translations_DIR}/${basename}_??.qm")
	else()
		unset(basename_files)
		foreach(_mapper_trans ${Mapper_translations})
			get_filename_component(basename_file ${_mapper_trans} NAME_WE)
			string(REPLACE OpenOrienteering ${basename} basename_file ${basename_file})
			string(REGEX REPLACE "(_..)_..$" "\\1" basename_file ${basename_file})
			file(GLOB translation_files RELATIVE "${Qt5Translations_DIR}" "${Qt5Translations_DIR}/${basename_file}*.qm")
			list(APPEND basename_files ${translation_files})
		endforeach()
	endif()
	
	install(CODE 
	  "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_CURRENT_BINARY_DIR}\" --target \"${basename}-translations\")"
	)
	
	set(generated_translations )
	foreach(basename_file ${basename_files})
		set(input_files "${Qt5Translations_DIR}/${basename_file}")
		foreach(arg ${ARGN})
			string(REPLACE ${basename} ${arg} extra_file ${basename_file})
			if(EXISTS "${Qt5Translations_DIR}/${extra_file}")
				list(INSERT input_files 0 "${Qt5Translations_DIR}/${extra_file}")
			endif()
		endforeach()
		add_custom_command(OUTPUT "${basename_file}"
		  COMMAND Qt5::lconvert -o "${basename_file}" ${input_files}
		  DEPENDS Qt5::lconvert ${input_files}
		)
		list(APPEND generated_translations "${basename_file}")
		install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${basename_file}" DESTINATION "${MAPPER_DATA_DESTINATION}/translations")
	endforeach()
	add_custom_target(${basename}-translations SOURCES ${generated_translations})
endfunction(deploy_qt_translations)



find_package(Qt5Core REQUIRED QUIET)
if(NOT QT_LIBRARY_DIR)
	get_target_property(_qt5core_lib Qt5::Core IMPORTED_LOCATION_RELEASE)
	get_filename_component(QT_LIBRARY_DIR ${_qt5core_lib} PATH)
	string(REGEX REPLACE "/[^/]*\\.framework" "" QT_LIBRARY_DIR "${QT_LIBRARY_DIR}")
endif()



# cf. http://www.cmake.org/cmake/help/cmake-2-8-docs.html#module:CPack
# cf. http://www.cmake.org/Wiki/CMake:CPackPackageGenerators
set(CPACK_PACKAGE_NAME "OpenOrienteering ${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "OpenOrienteering")
set(CPACK_PACKAGE_VERSION_MAJOR ${Mapper_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${Mapper_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${Mapper_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Map drawing program from OpenOrienteering")
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
if(ANDROID)
	set(_system_name "Android-${CMAKE_ANDROID_ARCH_ABI}")
elseif(APPLE)
	set(_system_name "macOS")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(_system_name "${CMAKE_SYSTEM_NAME}-x86")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_system_name "${CMAKE_SYSTEM_NAME}-x64")
else()
	set(_system_name "${CMAKE_SYSTEM_NAME}-unknown")
endif()
set(CPACK_PACKAGE_FILE_NAME 
    "OpenOrienteering-Mapper-${Mapper_VERSION_FILESYSTEM}-${_system_name}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_STRIP_FILES "TRUE")

set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "openorienteering-mapper_${Mapper_VERSION_FILESYSTEM}-src")
set(CPACK_SOURCE_IGNORE_FILES 
  "${PROJECT_BINARY_DIR}"
  "/[.]git/"
  "/3rd-party/clipper/download/"
  "/3rd-party/proj/download/"
  "/3rd-party/qt5/download/"
  ${CPACK_SOURCE_IGNORE_FILES}
)

set(MAPPER_MACOS_SUBDIR "")

set(CPACK_PACKAGE_EXECUTABLES "Mapper" "${CPACK_PACKAGE_NAME} ${Mapper_VERSION_DISPLAY}")

if(WIN32)
	# Packaging as ZIP archive
	set(CPACK_GENERATOR_DEFAULT "ZIP")
	#set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME} ${Mapper_VERSION_FILESYSTEM}")
	
	find_program(MAKENSIS_EXECUTABLE "makensis")
	if(MAKENSIS_EXECUTABLE)
		list(APPEND CPACK_GENERATOR_DEFAULT "NSIS")
		# The title displayed at the top of the installer
		set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
		# The display name string that appears in the Windows Add/Remove Program control panel
		set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${Mapper_VERSION_DISPLAY}")
		# NSIS start menu links will point to executables in this directory
		set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
		# A path to the executable that contains the uninstaller icon.
		set(CPACK_NSIS_INSTALLED_ICON_NAME Mapper.exe)
		# URL to a web site providing more information about your application.
		set(CPACK_NSIS_URL_INFO_ABOUT "https://www.openorienteering.org/apps/mapper/")
		# Extra NSIS include
		configure_file(windows/custom.nsi.in windows/custom.nsi @ONLY)
		set(CPACK_NSIS_DEFINES "!include \\\"${CMAKE_CURRENT_BINARY_DIR}\\\\windows\\\\custom.nsi\\\"")
		# Extra NSIS commands that will be added to the install/uninstall sections.
		set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS   "Call installAssociations")
		set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Call un.installAssociations")
		# 64 bit build
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
			set(CPACK_NSIS_DISPLAY_NAME "${CPACK_NSIS_DISPLAY_NAME} x64")
		endif()
	endif(MAKENSIS_EXECUTABLE)
	
elseif(APPLE)
	set(MAPPER_MACOS_SUBDIR "/Mapper.app/Contents/MacOS")
	set(CPACK_GENERATOR_DEFAULT "DragNDrop")
	set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/images/mapper-icon/Mapper.icns")
	set(CPACK_COMMAND_HDIUTIL "${CMAKE_CURRENT_LIST_DIR}/macos/hdiutil.sh")
	set_target_properties(Mapper PROPERTIES
	  MACOSX_BUNDLE_INFO_STRING          "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}"
	  MACOSX_BUNDLE_ICON_FILE            "Mapper.icns"
	  MACOSX_BUNDLE_GUI_IDENTIFIER       "org.openorienteering.${CMAKE_PROJECT_NAME}"
	  MACOSX_BUNDLE_LONG_VERSION_STRING  "${CMAKE_PROJECT_NAME} ${Mapper_VERSION_DISPLAY} for macOS"
	  MACOSX_BUNDLE_BUNDLE_NAME          "${CMAKE_PROJECT_NAME}" # less than 16 characters long
	  MACOSX_BUNDLE_SHORT_VERSION_STRING "${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}"
	  MACOSX_BUNDLE_BUNDLE_VERSION       "${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}"
	  MACOSX_BUNDLE_COPYRIGHT            "${Mapper_COPYRIGHT}"
	)
	install(FILES "${CPACK_PACKAGE_ICON}"
	  DESTINATION "${MAPPER_DATA_DESTINATION}"
	)
	
elseif(ANDROID)
	set(CPACK_GENERATOR_DEFAULT "")
	# Nothing else here: We always add the 'android' subdirectory.
	
elseif(UNIX AND EXISTS /usr/bin/dpkg AND EXISTS /usr/bin/lsb_release)
	# Packaging on Debian or similar
	set(CPACK_GENERATOR_DEFAULT "DEB")
	set(CPACK_DEBIAN_PACKAGE_NAME "openorienteering-mapper")
	execute_process(
	  COMMAND /usr/bin/lsb_release -sc 
	  OUTPUT_VARIABLE CPACK_LSB_RELEASE 
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	string(REPLACE "Linux-x86" "${CPACK_LSB_RELEASE}_i386"
	  CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}"
	)
	string(REPLACE "Linux-x64" "${CPACK_LSB_RELEASE}_amd64"
	  CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}"
	)
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kai Pastor <dg0yt@darc.de>")
	set(CPACK_DEBIAN_SECTION            "graphics")
	set(CPACK_DEBIAN_PACKAGE_HOMEPAGE   "https://www.openorienteering.org/apps/mapper/")
	
endif()

if(CPACK_GENERATOR_DEFAULT)
	set(CPACK_GENERATOR "${CPACK_GENERATOR_DEFAULT}"
		CACHE STRING "The binary package generators (ZIP;DEB;DragNDrop;NSIS)")
	set(CPACK_SOURCE_GENERATOR "OFF"
		CACHE STRING "The source package generators (TGZ;ZIP)")
	mark_as_advanced(CPACK_GENERATOR CPACK_SOURCE_GENERATOR)
	
	include(CPack)
endif()



# Cleanup obsolete cache items
unset(MAPPER_LIBS CACHE)
unset(MAPPER_QT_PLUGINS CACHE)

unset(MAPPER_LIB_HINTS)
unset(MAPPER_LIBS)

if(Mapper_PACKAGE_PROJ)
	if(NOT PROJ_DATA_DIR)
		if(PROJ_DIR)
			# Cf. find_package documentation
			string(REGEX REPLACE "/CMake$|/cmake$" "" proj_prefix "${PROJ_DIR}")              # U
			string(REGEX REPLACE "/PROJ[^/]*$|/proj[^/]*$" "" proj_prefix "${proj_prefix}")   # U, W
			string(REGEX REPLACE "/cmake$" "" proj_prefix "${proj_prefix}")                   # U
			string(REGEX REPLACE "/lib/[^/]*$|/lib$|/share$" "" proj_prefix "${proj_prefix}") # U, W
		elseif(PROJ4_PC_PREFIX)
			set(proj_prefix "${PROJ4_PC_PREFIX}")
		else()
			set(proj_prefix "${CMAKE_INSTALL_PREFIX}")
		endif()
		find_path(PROJ_DATA_DIR
		  DOC "The PROJ data files directory"
		  NAMES epsg proj.db
		  PATHS ${proj_prefix}
		  PATH_SUFFIXES "share/proj" "share"
		  NO_DEFAULT_PATH
		)
	endif()
	if(NOT PROJ_DATA_DIR)
		message(SEND_ERROR
		  "PROJ_DATA_DIR must be found or set "
		  "when Mapper_BUILD_PACKAGE and Mapper_PACKAGE_PROJ are enabled."
		)
	endif()
	install(
	  DIRECTORY "${PROJ_DATA_DIR}"
	  DESTINATION "${MAPPER_DATA_DESTINATION}")
	list(APPEND MAPPER_LIB_HINTS "${proj_prefix}/bin")
endif()

if(Mapper_PACKAGE_GDAL)
	if(NOT GDAL_DATA_DIR)
		unset(GDAL_CONFIG CACHE)
		find_program(GDAL_CONFIG gdal-config ONLY_CMAKE_FIND_ROOT_PATH)
		if(GDAL_CONFIG)
			execute_process(
			  COMMAND "${GDAL_CONFIG}" --datadir
			  OUTPUT_VARIABLE gdal_data_dir
			  OUTPUT_STRIP_TRAILING_WHITESPACE
			  RESULT_VARIABLE gdal_config_result
			)
		endif()
		if(GDAL_CONFIG AND gdal_config_result EQUAL 0)
			set(gdal_data_paths "${gdal_data_dir}")
		elseif(GDAL_INCLUDE_DIRS)
			string(REGEX REPLACE "/include$" "/share/gdal" gdal_data_paths "${GDAL_INCLUDE_DIRS}") # MSYS2 et al.
		else()
			set(gdal_data_paths PATHS "${CMAKE_INSTALL_PREFIX}/share/gdal")
		endif()
		find_path(GDAL_DATA_DIR
		  DOC "The GDAL data files directory"
		  NAMES ellipsoid.csv gdalvrt.xsd
		  PATHS ${gdal_data_paths}
		  NO_DEFAULT_PATH
		)
	endif()
	if(NOT GDAL_DATA_DIR)
		message(SEND_ERROR
		  "The gdal-config script must be available, "
		  "or GDAL_DATA_DIR must be set, "
		  "when Mapper_BUILD_PACKAGE and Mapper_PACKAGE_GDAL are enabled."
		)
	endif()
	install(
	  DIRECTORY "${GDAL_DATA_DIR}/"
	  DESTINATION "${MAPPER_DATA_DESTINATION}/gdal")
	foreach(item IN LISTS GDAL_LIBRARIES)
		if(EXISTS "${item}")
			get_filename_component(GDAL_LIBRARY_DIR "{item}" PATH)
			list(APPEND MAPPER_LIB_HINTS "${GDAL_LIBRARY_DIR}")
		endif()
	endforeach()
endif()

unset(MAPPER_QT_PLUGINS)
if(Mapper_PACKAGE_QT)
	set(QT_LIB_SUFFIX "")
	list(APPEND MAPPER_LIB_HINTS ${QT_LIBRARY_DIR})
	set(MAPPER_QT_PLUGINS
	  generic/qevdevkeyboardplugin
	  generic/qevdevmouseplugin
	  generic/qevdevtabletplugin
	  generic/qevdevtouchplugin
	  imageformats/qgif
	  imageformats/qicns
	  imageformats/qico
	  imageformats/qjp2
	  imageformats/qjpeg
	  imageformats/qtiff
	  imageformats/qwebp
	  platforminputcontexts/composeplatforminputcontextplugin
	  platforminputcontexts/ibusplatforminputcontextplugin
	  platforms/qcocoa
	  platforms/qwindows
	  platforms/qxcb
	  position/qtposition_cl
	  position/qtposition_geoclue
	  position/qtposition_serialnmea
	  printsupport/cocoaprintersupport
	  printsupport/cupsprintersupport
	  printsupport/windowsprintersupport
	  styles/qmacstyle
	)
	set(MAPPER_QT_MODULES
	  Gui
	  Positioning
	  PrintSupport
	  Sensors
	  Sql
	  Widgets
	)

	if(ANDROID)
		set(MAPPER_QT_PLUGINS ) # Ignore for now, handled by androiddeployqt
		list(REMOVE_ITEM MAPPER_QT_MODULES Sql) # Not deploying Qt Assistant
	endif()
	
	find_package(Qt5Positioning QUIET)
	if(TARGET Qt5::Positioning)
		list(APPEND MAPPER_QT_MODULES SerialPort) # Always with serialport NMEA
	endif()
	
	set(QT_PLUGIN_TARGETS )
	foreach(module ${MAPPER_QT_MODULES})
		find_package(Qt5${module} QUIET)
		if(module STREQUAL "Positioning" AND Qt5Positioning_DIR)
			# Workaround for QTBUG-58812 CMake: Plugin config not loaded
			# unless plugin class name ends in Plugin
			file(GLOB pluginTargets "${Qt5Positioning_DIR}/Qt5Positioning_*.cmake")
			foreach(file ${pluginTargets})
				string(REGEX REPLACE "^.*/Qt5Positioning_([^/]*)\\.cmake" "Qt5::\\1" lib "${file}")
				if(NOT file MATCHES "Plugin.cmake"
				   AND NOT "${Qt5Positioning_PLUGINS}" MATCHES "${lib}")
					include(${file})
				endif()
			endforeach()
		endif()
		list(APPEND QT_PLUGIN_TARGETS ${Qt5${module}_PLUGINS})
	endforeach()
	# Cf. Qt5's qt_de.ts for dependencies - qt_de.ts would not load without them.
	deploy_qt_translations(qt qtbase)
endif()

if(Mapper_PACKAGE_ASSISTANT)
	set(assistant_find_options )
	if(CMAKE_FIND_ROOT_PATH)
		set(assistant_find_options ONLY_CMAKE_FIND_ROOT_PATH)
	endif()
	find_program(Qt5Help_ASSISTANT_EXECUTABLE
	  NAMES assistant Assistant assistant.exe
#	  HINTS bin
	  ${assistant_find_options}
	)
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
	endif()
	list(APPEND MAPPER_QT_PLUGINS
	  sqldrivers/qsqlite
	)

	deploy_qt_translations(assistant qt_help)
endif(Mapper_PACKAGE_ASSISTANT)

if(MINGW)
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
	# Grep is used (and desperately needed) to speed up objdump parsing.
	find_program(gp_grep_cmd NAMES grep)
endif()
	

# Install all plugins
unset(MAPPER_QT_PLUGINS_FOUND)
foreach(_qt_plugin ${MAPPER_QT_PLUGINS})
	string(REPLACE "/" "/lib" alternative_pattern "${_qt_plugin}")
	foreach(plugin_target ${QT_PLUGIN_TARGETS} NOT_FOUND)
		if(plugin_target STREQUAL "NOT_FOUND")
			message(STATUS "   ${_qt_plugin} plugin library - not found")
			break()
		endif()
		get_target_property(_qt_plugin_location ${plugin_target} IMPORTED_LOCATION_RELEASE)
		if(_qt_plugin_location MATCHES "${_qt_plugin}"
		   OR _qt_plugin_location MATCHES "${alternative_pattern}")
			message(STATUS "   ${_qt_plugin} plugin library - found")
			list(APPEND MAPPER_QT_PLUGINS_FOUND "${_qt_plugin}")
			get_filename_component(_qt_plugin_dir "${_qt_plugin}" PATH)
			if(APPLE)
				install(
				  FILES "${_qt_plugin_location}"
				  DESTINATION "${MAPPER_RUNTIME_DESTINATION}${MAPPER_MACOS_SUBDIR}/../PlugIns/${_qt_plugin_dir}")
			else()
				install(
				  FILES "${_qt_plugin_location}"
				  DESTINATION "${MAPPER_LIBRARY_DESTINATION}/plugins/${_qt_plugin_dir}")
			endif()
			break()
		endif()
	endforeach()
endforeach(_qt_plugin)


if(UNIX AND NOT APPLE AND NOT ANDROID)
	install(
	  FILES "${PROJECT_SOURCE_DIR}/doc/man/Mapper.1"
	  DESTINATION "share/man/man1")
	install(
	  FILES "${CMAKE_CURRENT_BINARY_DIR}/linux/Mapper.desktop"
	  DESTINATION "share/applications")
	install(
	  FILES "${CMAKE_CURRENT_BINARY_DIR}/linux/openorienteering-mapper.xml"
	  DESTINATION "share/mime/packages")
	# Cf. http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html#directory_layout
	foreach(_size 16 24 32 48 96 128 256 512)
		install(
		  FILES "${PROJECT_SOURCE_DIR}/images/mapper-icon/Mapper-${_size}.png"
		  DESTINATION "share/icons/hicolor/${_size}x${_size}/apps"
		  RENAME Mapper.png
		)
		install(
		  FILES "${PROJECT_SOURCE_DIR}/images/mapper-icon/Mapper-${_size}.png"
		  DESTINATION "share/icons/hicolor/${_size}x${_size}/mimetypes"
		  RENAME application-x-openorienteering-xmap.png
		)
		install(
		  FILES "${PROJECT_SOURCE_DIR}/images/mapper-icon/Mapper-${_size}.png"
		  DESTINATION "share/icons/hicolor/${_size}x${_size}/mimetypes"
		  RENAME application-x-openorienteering-ocd.png
		)
	endforeach()
endif()

# By expanding all @VAR, custom_install.cmake makes the install more traceable.
configure_file("custom_install.cmake.in" "custom_install.cmake" @ONLY)
install(CODE "include(\"${CMAKE_CURRENT_BINARY_DIR}/custom_install.cmake\")")


# Always include the android directory, for convenient development.
add_subdirectory("android")
