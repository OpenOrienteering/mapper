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

	# sources not properly configured
	set(CPACK_SOURCE_GENERATOR "OFF")
	
	# cf. http://www.cmake.org/cmake/help/cmake-2-8-docs.html#module:CPack
	# cf. http://www.cmake.org/Wiki/CMake:CPackPackageGenerators
	set(CPACK_PACKAGE_NAME "OpenOrienteering Mapper")
	set(CPACK_PACKAGE_VENDOR "OpenOrienteering Developers")
	set(CPACK_PACKAGE_VERSION_MAJOR ${Mapper_VERSION_MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${Mapper_VERSION_MINOR})
	set(CPACK_PACKAGE_VERSION_PATCH ${Mapper_VERSION_PATCH})
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY 
	  "Map drawing program from OpenOrienteering")
	set(CPACK_PACKAGE_FILE_NAME 
	  "openorienteering-mapper_${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CMAKE_SYSTEM_PROCESSOR}")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
	set(CPACK_STRIP_FILES "TRUE")
	
	if(WIN32)
		# Packaging as ZIP archive
		set(CPACK_GENERATOR "ZIP")
		set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
		set(CPACK_PACKAGING_INSTALL_PREFIX "/Mapper-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
	endif(WIN32)
	
	if(UNIX AND EXISTS /usr/bin/dpkg AND EXISTS /usr/bin/lsb_release)
		# Packaging on Debian or similar
		set(CPACK_GENERATOR "DEB")
		execute_process(
		  COMMAND /usr/bin/lsb_release -sc 
		  OUTPUT_VARIABLE CPACK_LSB_RELEASE 
		  OUTPUT_STRIP_TRAILING_WHITESPACE)
		string(REPLACE
		  ${CMAKE_SYSTEM_PROCESSOR} 
		  "${CPACK_LSB_RELEASE}_${CMAKE_SYSTEM_PROCESSOR}" 
		  CPACK_PACKAGE_FILE_NAME
		  ${CPACK_PACKAGE_FILE_NAME})
		string(REPLACE 
		  "x86_64" 
		  "amd64" 
		  CPACK_PACKAGE_FILE_NAME
		  ${CPACK_PACKAGE_FILE_NAME})
		set(CPACK_DEBIAN_PACKAGE_NAME "openorienteering-mapper")
 		add_definitions(-DMAPPER_DEBIAN_PACKAGE_NAME="${CPACK_DEBIAN_PACKAGE_NAME}")
		set(CPACK_DEBIAN_PACKAGE_MAINTAINER
		   "OpenOrienteering Developers <dg0yt@darc.de>")
		set(CPACK_DEBIAN_SECTION "graphics")
		set(CPACK_DEBIAN_PACKAGE_HOMEPAGE 
		  "http://oorienteering.sourceforge.net/")
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
	
	endif(UNIX AND EXISTS /usr/bin/dpkg AND EXISTS /usr/bin/lsb_release)
	
	include(CPack)
	
endif(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

if(WIN32)
	set(MAPPER_RUNTIME_DESTINATION .)
	set(MAPPER_DATA_DESTINATION .)
	set(MAPPER_ABOUT_DESTINATION "doc")
else(WIN32)
	set(MAPPER_RUNTIME_DESTINATION bin)
	set(MAPPER_DATA_DESTINATION "share/${CPACK_DEBIAN_PACKAGE_NAME}")
	set(MAPPER_ABOUT_DESTINATION "share/doc/${CPACK_DEBIAN_PACKAGE_NAME}")
endif(WIN32)

install(
  TARGETS Mapper
  RUNTIME DESTINATION "${MAPPER_RUNTIME_DESTINATION}")
install(
  FILES COPYING 
  DESTINATION "${MAPPER_ABOUT_DESTINATION}")
install(
  FILES "bin/my symbol sets/4000/ISSOM_4000.omap"
  DESTINATION "${MAPPER_DATA_DESTINATION}/symbol sets/4000")
install(
  FILES "bin/my symbol sets/5000/ISSOM_5000.omap"
  DESTINATION "${MAPPER_DATA_DESTINATION}/symbol sets/5000")
install(
  FILES "bin/my symbol sets/10000/ISOM_10000.omap"
  DESTINATION "${MAPPER_DATA_DESTINATION}/symbol sets/10000")
install(
  FILES "bin/my symbol sets/15000/ISOM_15000.omap"
  DESTINATION "${MAPPER_DATA_DESTINATION}/symbol sets/15000")
if(NOT Mapper_TRANSLATIONS_EMBEDDED)
install(
  DIRECTORY "bin/translations/"
  DESTINATION "${MAPPER_DATA_DESTINATION}/translations"
  FILES_MATCHING PATTERN "*.qm")
endif(NOT Mapper_TRANSLATIONS_EMBEDDED)
install(
  FILES "bin/help/oomaphelpcollection.qhc" "bin/help/oomaphelp.qch"
  DESTINATION "${MAPPER_DATA_DESTINATION}/help")

if(WIN32)
	set(MAPPER_LIBS QtCore4 QtGui4 QtNetwork4 QtXml4 CACHE INTERNAL
	  "Qt-related libraries which need to be deployed to the package")
	if(MINGW)
		list(APPEND MAPPER_LIBS libgcc_s_dw2-1 mingwm10)
	endif(MINGW)
		
	set(MAPPER_LIB_FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libproj-0.dll" CACHE INTERNAL
	  "The Proj.4 library which needs to be deployed to the package")
	foreach(MAPPER_LIBRARY ${MAPPER_LIBS})
		unset(MAPPER_LIBRARY_PATH CACHE)
		find_library(MAPPER_LIBRARY_PATH ${MAPPER_LIBRARY})
		if(NOT MAPPER_LIBRARY_PATH)
			message(FATAL_ERROR "Library not found: ${MAPPER_LIBRARY}")
		endif(NOT MAPPER_LIBRARY_PATH)
		list(APPEND MAPPER_LIB_FILES "${MAPPER_LIBRARY_PATH}")
	endforeach(MAPPER_LIBRARY)
	unset(MAPPER_LIBRARY_PATH CACHE)
	install(
	  FILES ${MAPPER_LIB_FILES} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}")
	
	unset(MAPPER_QT_PLUGINS CACHE)
	foreach(MAPPER_IMAGEFORMAT qgif4 qjpeg4 qmng4 qsvg4 qtga4 qtiff4)
		unset(MAPPER_PLUGIN_PATH CACHE)
		find_library(MAPPER_PLUGIN_PATH ${MAPPER_IMAGEFORMAT} PATH_SUFFIXES ../plugins/imageformats)
		if(NOT MAPPER_PLUGIN_PATH)
			message(FATAL_ERROR "Plugin not found: ${MAPPER_IMAGEFORMAT}")
		endif(NOT MAPPER_PLUGIN_PATH)
		list(APPEND MAPPER_QT_PLUGINS "${MAPPER_PLUGIN_PATH}")
	endforeach(MAPPER_IMAGEFORMAT)
	unset(MAPPER_PLUGIN_PATH CACHE)
	install(
	  FILES ${MAPPER_QT_PLUGINS} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}/plugins/imageformats")
	  
	unset(MAPPER_QT_TRANSLATIONS CACHE)
	foreach(MAPPER_QT_TRANSLATION de ja nb sv uk)
		set(MAPPER_QT_TRANSLATION_PATH "${QT_TRANSLATIONS_DIR}/qt_${MAPPER_QT_TRANSLATION}.qm")
		if(NOT EXISTS "${MAPPER_QT_TRANSLATION_PATH}")
			message(FATAL_ERROR "Qt translation not found: ${MAPPER_QT_TRANSLATION}")
		endif()
		list(APPEND MAPPER_QT_TRANSLATIONS "${MAPPER_QT_TRANSLATION_PATH}")
	endforeach(MAPPER_QT_TRANSLATION)
	install(
	  FILES ${MAPPER_QT_TRANSLATIONS} 
	  DESTINATION "${MAPPER_RUNTIME_DESTINATION}/translations")
endif(WIN32)

if(UNIX AND NOT APPLE AND NOT CYGWIN)
	install(
	  FILES "debian/Mapper.desktop"
	  DESTINATION "share/applications")
	install(
	  FILES "debian/Mapper.xpm"
	  DESTINATION "share/pixmaps")
endif(UNIX AND NOT APPLE AND NOT CYGWIN)
