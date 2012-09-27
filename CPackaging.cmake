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

	include(InstallRequiredSystemLibraries)

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
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
	set(CPACK_STRIP_FILES "TRUE")
	
	set(CPACK_SOURCE_GENERATOR "OFF"
	  CACHE STRING "The source package generators (TGZ;ZIP)")
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
		set(CPACK_GENERATOR "ZIP")
		#set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
		set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenOrienteering Mapper ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
		set(CPACK_PACKAGE_EXECUTABLES "Mapper" "OpenOrienteering Mapper ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

		find_program(MAKENSIS_EXECUTABLE "makensis")
		if(MAKENSIS_EXECUTABLE)
			list(APPEND CPACK_GENERATOR "NSIS")
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
		set(CPACK_GENERATOR "DragNDrop")
		set(CPACK_PACKAGE_EXECUTABLES "Mapper" "OpenOrienteering Mapper ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
		set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macosx/Mapper.icns")
		set(MAPPER_MACOS_SUBDIR "/Mapper.app/Contents/MacOS")

#		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/packaging/macosx/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
#		set(CPACK_BUNDLE_NAME "Mapper")
#		set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macosx/Mapper.icns")
#		set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
#		set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macosx/Mapper.sh")

	elseif(UNIX AND EXISTS /usr/bin/dpkg AND EXISTS /usr/bin/lsb_release)
		# Packaging on Debian or similar
		set(CPACK_GENERATOR "DEB")
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

	include(CPack)
	
endif(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

if(WIN32 OR APPLE)
	message("-- Checking extra files needed for standalone packaging")

	set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
	include(InstallRequiredSystemLibraries)

	find_program(QT_QTASSISTANT_EXECUTABLE assistant assistant.exe
	  DOC "The path of the Qt Assistant executable. Qt Assistant will not be bundled if this path is empty."
	  HINTS ${QT_BINARY_DIR}
	  NO_CMAKE_FIND_ROOT_PATH
	  NO_DEFAULT_PATH)
	if(QT_QTASSISTANT_EXECUTABLE)
		message("   Qt Assistant - found")
		install(
		  PROGRAMS ${QT_QTASSISTANT_EXECUTABLE}
		  DESTINATION "${MAPPER_RUNTIME_DESTINATION}${MAPPER_MACOS_SUBDIR}")
		if (NOT APPLE)
		install(
		  FILES "3rd-party/qt4/qt.conf"
		  DESTINATION "${MAPPER_RUNTIME_DESTINATION}")
		endif()
	else()
		message("   Qt Assistant - not found")
	endif()
	mark_as_advanced(QT_QTASSISTANT_EXECUTABLE)

	if(PROJ_BINARY_DIR)
		install(
		  DIRECTORY "${PROJ_BINARY_DIR}/../share/proj"
		  DESTINATION "${MAPPER_DATA_DESTINATION}")
	endif(PROJ_BINARY_DIR)
endif(WIN32 OR APPLE)

if(WIN32)
	set(MAPPER_LIBS proj-0 QtCore4 QtGui4 QtNetwork4 QtXml4 CACHE INTERNAL
	  "The libraries which need to be deployed to the package")
	if(QT_QTASSISTANT_EXECUTABLE)
		list(APPEND MAPPER_LIBS QtHelp4 QtCLucene4 QtSql4 QtWebKit4)
	endif(QT_QTASSISTANT_EXECUTABLE)
	if(TOOLCHAIN_SHARED_LIBS)
		list(APPEND MAPPER_LIBS ${TOOLCHAIN_SHARED_LIBS})
	elseif(MINGW)
		list(APPEND MAPPER_LIBS libgcc_s_dw2-1 mingwm10)
	endif()
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
	
	unset(MAPPER_QT_IMAGEFORMATS CACHE)
	foreach(_qt_imageformat qgif4 qjpeg4 qmng4 qsvg4 qtga4 qtiff4)
		unset(_qt_imageformat_path CACHE)
		find_library(_qt_imageformat_path ${_qt_imageformat}
		  HINTS ${QT_BINARY_DIR} NO_CMAKE_FIND_ROOT_PATH
		  PATH_SUFFIXES ../plugins/imageformats)
		if(_qt_imageformat_path)
			message("   ${_qt_imageformat} DLL (plugin) - found")
			list(APPEND MAPPER_QT_IMAGEFORMATS "${_qt_imageformat_path}")
		else()
			message("   ${_qt_imageformat} DLL (plugin) - not found")
		endif(_qt_imageformat_path)
	endforeach(_qt_imageformat)
	unset(_qt_imageformat_path CACHE)
	install(
	  FILES ${MAPPER_QT_IMAGEFORMATS} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}/plugins/imageformats")

if(QT_QTASSISTANT_EXECUTABLE)
	unset(MAPPER_QT_SQLDRIVERS CACHE)
	foreach(_qt_sqldriver qsqlite4)
		unset(_qt_sqldriver_path CACHE)
		find_library(_qt_sqldriver_path ${_qt_sqldriver}
		  HINTS ${QT_BINARY_DIR} NO_CMAKE_FIND_ROOT_PATH
		  PATH_SUFFIXES ../plugins/sqldrivers)
		if(_qt_sqldriver_path)
			message("   ${_qt_sqldriver} DLL (plugin) - found")
			list(APPEND MAPPER_QT_SQLDRIVERS "${_qt_sqldriver_path}")
		else()
			message("   ${_qt_sqldriver} DLL (plugin) - not found")
		endif(_qt_sqldriver_path)
	endforeach(_qt_sqldriver)
	unset(_qt_sqldriver_path CACHE)
	install(
	  FILES ${MAPPER_QT_SQLDRIVERS} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}/plugins/sqldrivers")
endif(QT_QTASSISTANT_EXECUTABLE)

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
	install(
	  FILES ${MAPPER_QT_TRANSLATIONS} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}/translations")

	message("-- Checking system files needed for standalone packaging - done")
endif(WIN32)

if(UNIX AND NOT APPLE AND NOT CYGWIN)
	install(
	  FILES "debian/Mapper.desktop"
	  DESTINATION "share/applications")
	install(
	  FILES "debian/Mapper.xpm"
	  DESTINATION "share/pixmaps")
endif(UNIX AND NOT APPLE AND NOT CYGWIN)
