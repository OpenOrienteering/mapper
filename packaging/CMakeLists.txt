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
	  "${CMAKE_CURRENT_BINARY_DIR}"
	  "/docs/"
	  "/[.]git/"
	  "/3rd-party/clipper/"
	  "/3rd-party/proj/download/"
	  "/3rd-party/qt4/download/"
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
			  ExecWait '\\\"$1\\\" /C ftype OpenOrienteering Map=\\\"$INSTDIR\\\\Mapper.exe\\\" \\\"%1\\\"' $0)
			string(REPLACE ";" " " _nsis_extra_inst "${_nsis_extra_inst}")
			set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS ${_nsis_extra_inst})
			set(_nsis_extra_uninst 
			  ReadEnvStr $1 ComSpec\n
			  ExecWait '\\\"$1\\\" /C ftype OpenOrienteering Map= ' $0\n
			  ExecWait '\\\"$1\\\" /C assoc .omap=' $0)
			string(REPLACE ";" " " _nsis_extra_uninst "${_nsis_extra_uninst}")
			set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${_nsis_extra_uninst})
		endif(MAKENSIS_EXECUTABLE)

	elseif(APPLE)
		set(MAPPER_MACOS_SUBDIR "/Mapper.app/Contents/MacOS")
		set(CPACK_GENERATOR_DEFAULT "DragNDrop")
		set(CPACK_PACKAGE_EXECUTABLES "Mapper" "OpenOrienteering Mapper ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
		set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/macosx/Mapper.icns")
		set_target_properties(Mapper PROPERTIES
		  MACOSX_BUNDLE_INFO_STRING          "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}"
		  MACOSX_BUNDLE_ICON_FILE            "Mapper.icns"
		  MACOSX_BUNDLE_GUI_IDENTIFIER       "net.sourceforge.oorienteering.Mapper"
		  MACOSX_BUNDLE_LONG_VERSION_STRING  "Mapper ${Mapper_VERSION_DISPLAY} for Mac OS X"
		  MACOSX_BUNDLE_BUNDLE_NAME          "Mapper" # less than 16 characters long
		  MACOSX_BUNDLE_SHORT_VERSION_STRING "${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}"
		  MACOSX_BUNDLE_BUNDLE_VERSION       "${Mapper_VERSION_MAJOR}.${Mapper_VERSION_MINOR}.${Mapper_VERSION_PATCH}"
		  MACOSX_BUNDLE_COPYRIGHT            "(C) 2012 OpenOrienteering developers"

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
 		add_definitions(-DMAPPER_DEBIAN_PACKAGE_NAME="${CPACK_DEBIAN_PACKAGE_NAME}")
		set(CPACK_DEBIAN_PACKAGE_MAINTAINER
		   "Kai Pastor <dg0yt@darc.de>")
		set(CPACK_DEBIAN_SECTION "graphics")
		set(CPACK_DEBIAN_PACKAGE_HOMEPAGE 
		  "http://oorienteering.sourceforge.net/?cat=3")
		set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
		set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt4-dev-tools")
		
		unset(FAKEROOT_EXECUTABLE CACHE)
		find_program(FAKEROOT_EXECUTABLE fakeroot)
		if(NOT FAKEROOT_EXECUTABLE)
			install(CODE "MESSAGE(WARNING
			  \"'fakeroot' not found. To build a DEB package with proper file \"
			  \"ownership, fakeroot must be installed.\")")
		endif(NOT FAKEROOT_EXECUTABLE)
		mark_as_advanced(FAKEROOT_EXECUTABLE)
		
		# workaround for https://bugs.launchpad.net/ubuntu/+source/cmake/+bug/972419
		if (CPACK_LSB_RELEASE STREQUAL "precise")
			install(CODE "MESSAGE(WARNING 
			  \"Ubuntu 12.04 (${CPACK_LSB_RELEASE}) has a broken DEB package generator \"
			  \"(cf. https://bugs.launchpad.net/ubuntu/+source/cmake/+bug/972419).\n\"
			  \"Run 'make package_repair' if the DEB package installation fails with \"
			  \"the message 'corrupted filesystem tarfile'.\")")
 		endif (CPACK_LSB_RELEASE STREQUAL "precise")
		add_custom_target(package_repair
		  COMMENT "Rebuilding DEB package"
		  COMMAND dpkg-deb -R "${CPACK_PACKAGE_FILE_NAME}.deb" "${CPACK_PACKAGE_FILE_NAME}"
		  COMMAND /usr/bin/fakeroot dpkg-deb -b "${CPACK_PACKAGE_FILE_NAME}"
		  COMMAND rm -R "${CPACK_PACKAGE_FILE_NAME}")
	
	endif()

	set(CPACK_GENERATOR "${CPACK_GENERATOR_DEFAULT}"
	  CACHE STRING "The binary package generators (ZIP;DEB;DragNDrop;NSIS)")
	set(CPACK_SOURCE_GENERATOR "OFF"
	  CACHE STRING "The source package generators (TGZ;ZIP)")
	mark_as_advanced(CPACK_GENERATOR CPACK_SOURCE_GENERATOR)

	include(CPack)
	
endif(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")


if(Mapper_PACKAGE_ASSISTANT)
	message(STATUS "Checking for Qt Assistant")
	find_program(QT_QTASSISTANT_EXECUTABLE assistant${CMAKE_EXECUTABLE_SUFFIX}
	  DOC "The path of the Qt Assistant executable"
	  HINTS ${QT_BINARY_DIR}
	  NO_CMAKE_FIND_ROOT_PATH
	  NO_DEFAULT_PATH)
	if(NOT QT_QTASSISTANT_EXECUTABLE)
		message(FATAL "Checking for Qt Assistant - not found")
	endif()
	message(STATUS "Checking for Qt Assistant - found: ${QT_QTASSISTANT_EXECUTABLE}")
	install(
	  PROGRAMS ${QT_QTASSISTANT_EXECUTABLE}
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}${MAPPER_MACOS_SUBDIR}")
	if(WIN32)
		install(
		  FILES "${PROJECT_SOURCE_DIR}/3rd-party/qt4/qt.conf"
		  DESTINATION "${MAPPER_RUNTIME_DESTINATION}")
	endif()
	mark_as_advanced(QT_QTASSISTANT_EXECUTABLE)
endif(Mapper_PACKAGE_ASSISTANT)


if(Mapper_PACKAGE_LIBRARIES)
	if(WIN32)
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
	endif()
	include(InstallRequiredSystemLibraries)

	if(PROJ_BINARY_DIR)
		install(
		  DIRECTORY "${PROJ_BINARY_DIR}/../share/proj"
		  DESTINATION "${MAPPER_DATA_DESTINATION}")
	endif(PROJ_BINARY_DIR)

	set(PROJ_LIB_SUFFIX "")
	set(QT_LIB_SUFFIX "")
	if (WIN32)
		set(PROJ_LIB_SUFFIX -0)
		set(QT_LIB_SUFFIX 4)
	endif(WIN32)

	set(MAPPER_LIBS proj${PROJ_LIB_SUFFIX} QtCore${QT_LIB_SUFFIX} QtGui${QT_LIB_SUFFIX} QtNetwork${QT_LIB_SUFFIX} QtXml${QT_LIB_SUFFIX}
	  CACHE INTERNAL "The dynamic libraries which need to be deployed")
	set(MAPPER_PLUGINS_IMAGEFORMATS qgif${QT_LIB_SUFFIX} qjpeg${QT_LIB_SUFFIX} qmng${QT_LIB_SUFFIX} qsvg${QT_LIB_SUFFIX} qtga${QT_LIB_SUFFIX} qtiff${QT_LIB_SUFFIX}
	  CACHE INTERNAL "The image format plugins which need to be deployed")
	set(MAPPER_PLUGINS_SQLDRIVERS ""
	  CACHE INTERNAL "The sql driver plugins which need to be deployed")

	if(Mapper_PACKAGE_ASSISTANT)
		list(APPEND MAPPER_LIBS QtHelp${QT_LIB_SUFFIX} QtCLucene${QT_LIB_SUFFIX} QtSql${QT_LIB_SUFFIX} QtWebKit${QT_LIB_SUFFIX})
		list(APPEND MAPPER_PLUGINS_SQLDRIVERS qsqlite${QT_LIB_SUFFIX})
	endif(Mapper_PACKAGE_ASSISTANT)

	if(TOOLCHAIN_SHARED_LIBS)
		list(APPEND MAPPER_LIBS ${TOOLCHAIN_SHARED_LIBS})
	elseif(MINGW)
		list(APPEND MAPPER_LIBS libgcc_s_dw2-1 mingwm10)
	endif()
endif()

if(Mapper_PACKAGE_LIBRARIES)
	foreach(_mapper_lib ${MAPPER_LIBS})
		unset(_mapper_lib_path CACHE)
		find_library(_mapper_lib_path ${_mapper_lib}
		  HINTS ${PROJ_BINARY_DIR} ${QT_BINARY_DIR}
		  PATH_SUFFIXES ${TOOLCHAIN_PATH_SUFFIXES}
		  NO_CMAKE_FIND_ROOT_PATH)
		get_filename_component(_mapper_lib_ext "${_mapper_lib_path}" EXT)
		if(_mapper_lib_ext STREQUAL ".dll")
			message("   ${_mapper_lib} DLL - found")
			list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${_mapper_lib_path}")
		else()
			message("   ${_mapper_lib} DLL - not found")
		endif()
	endforeach(_mapper_lib)
	unset(_mapper_lib_path CACHE)
	install(
	  FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}")
	
	unset(_bundle_plugins)

	unset(MAPPER_QT_IMAGEFORMATS CACHE)
	foreach(_qt_imageformat ${MAPPER_PLUGINS_IMAGEFORMATS})
		unset(_qt_imageformat_path CACHE)
		find_library(_qt_imageformat_path ${_qt_imageformat}
		  HINTS ${QT_PLUGINS_DIR} NO_CMAKE_FIND_ROOT_PATH
		  PATH_SUFFIXES imageformats)
		if(_qt_imageformat_path)
			message("   ${_qt_imageformat} DLL (plugin) - found")
			list(APPEND MAPPER_QT_IMAGEFORMATS "${_qt_imageformat_path}")
			list(APPEND _bundle_plugins "${_qt_imageformat_path}")
		else()
			message("   ${_qt_imageformat} DLL (plugin) - not found")
		endif(_qt_imageformat_path)
	endforeach(_qt_imageformat)
	unset(_qt_imageformat_path CACHE)
if(WIN32)
	install(
	  FILES ${MAPPER_QT_IMAGEFORMATS} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}/plugins/imageformats")
endif(WIN32)

	unset(MAPPER_QT_SQLDRIVERS CACHE)
	foreach(_qt_sqldriver ${MAPPER_PLUGINS_SQLDRIVERS})
		unset(_qt_sqldriver_path CACHE)
		find_library(_qt_sqldriver_path ${_qt_sqldriver}
		  HINTS ${QT_PLUGINS_DIR} NO_CMAKE_FIND_ROOT_PATH
		  PATH_SUFFIXES sqldrivers)
		if(_qt_sqldriver_path)
			message("   ${_qt_sqldriver} DLL (plugin) - found")
			list(APPEND MAPPER_QT_SQLDRIVERS "${_qt_sqldriver_path}")
			list(APPEND _bundle_plugins "${_qt_sqldriver_path}")
		else()
			message("   ${_qt_sqldriver} DLL (plugin) - not found")
		endif(_qt_sqldriver_path)
	endforeach(_qt_sqldriver)
	unset(_qt_sqldriver_path CACHE)
if(WIN32)
	install(
	  FILES ${MAPPER_QT_SQLDRIVERS} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}/plugins/sqldrivers")
endif(WIN32)
endif()

if(Mapper_PACKAGE_LIBRARIES AND APPLE)
	# use updated DeployQt4.cmake from CMake git repository
	include(${PROJECT_SOURCE_DIR}/cmake/DeployQt4.cmake)
	install_qt4_executable(Mapper.app 
	  "${MAPPER_PLUGINS_IMAGEFORMATS};${MAPPER_PLUGINS_SQLDRIVERS}"
	  "Mapper.app/Contents/MacOS/assistant")
endif()


if(Mapper_PACKAGE_TRANSLATIONS)
	message(STATUS "Checking for Qt translations")
	unset(MAPPER_QT_TRANSLATIONS CACHE)
	foreach(_mapper_trans ${Mapper_TRANS})
		get_filename_component(_qt_translation ${_mapper_trans} NAME_WE)
		unset(_found_translations)
		string(REPLACE OpenOrienteering_ "" _mapper_lang ${_qt_translation})
		string(REPLACE OpenOrienteering qt _qt_translation ${_qt_translation})
		set(_qt_translation_path "${QT_TRANSLATIONS_DIR}/${_qt_translation}.qm")
		if(EXISTS "${_qt_translation_path}")
			list(APPEND MAPPER_QT_TRANSLATIONS "${_qt_translation_path}")
			list(APPEND _found_translations ${_qt_translation})
		endif()
		if(Mapper_PACKAGE_ASSISTANT)
			string(REPLACE qt qt_help _qt_translation ${_qt_translation})
			set(_qt_translation_path "${QT_TRANSLATIONS_DIR}/${_qt_translation}.qm")
			if(EXISTS "${_qt_translation_path}")
				list(APPEND MAPPER_QT_TRANSLATIONS "${_qt_translation_path}")
				list(APPEND _found_translations ${_qt_translation})
			endif()
			string(REPLACE qt_help assistant _qt_translation ${_qt_translation})
			set(_qt_translation_path "${QT_TRANSLATIONS_DIR}/${_qt_translation}.qm")
			if(EXISTS "${_qt_translation_path}")
				list(APPEND MAPPER_QT_TRANSLATIONS "${_qt_translation_path}")
				list(APPEND _found_translations ${_qt_translation})
			endif()
		endif(Mapper_PACKAGE_ASSISTANT)
		if (_found_translations)
			message(STATUS "Checking for Qt translations - ${_mapper_lang} found: ${_found_translations}")
		else()
			message(STATUS "Checking for Qt translations - ${_mapper_lang} not found")
		endif()
	endforeach(_mapper_trans)
	install(
	  FILES ${MAPPER_QT_TRANSLATIONS} 
	  DESTINATION "${MAPPER_DATA_DESTINATION}/translations")
endif(Mapper_PACKAGE_TRANSLATIONS)


if(UNIX AND NOT APPLE AND NOT CYGWIN)
	install(
	  FILES "debian/Mapper.desktop"
	  DESTINATION "share/applications")
	install(
	  FILES "debian/Mapper.xpm"
	  DESTINATION "share/pixmaps")
endif(UNIX AND NOT APPLE AND NOT CYGWIN)

