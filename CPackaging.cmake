# Packaging with CPack
if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

	include(InstallRequiredSystemLibraries)

	# sources not properly configured
	set(CPACK_SOURCE_GENERATOR "OFF")

	# cf. http://www.cmake.org/cmake/help/cmake-2-8-docs.html#module:CPack
	# cf. http://www.cmake.org/Wiki/CMake:CPackPackageGenerators
	set(CPACK_PACKAGE_Name "OpenOrienteering Mapper")
	set(CPACK_PACKAGE_VENDOR "OpenOrienteering Developers")
	set(CPACK_PACKAGE_VERSION_MAJOR "0")
	set(CPACK_PACKAGE_VERSION_MINOR "alpha-2")
	set(CPACK_PACKAGE_VERSION_PATCH "1")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY 
	  "Map drawing program from OpenOrienteering")
	set(CPACK_PACKAGE_FILE_NAME 
	  "openorienteering-mapper_${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CMAKE_SYSTEM_PROCESSOR}")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
	set(CPACK_STRIP_FILES "TRUE")

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
		set(CPACK_DEBIAN_PACKAGE_MAINTAINER
		   "OpenOrienteering Developers <dg0yt@darc.de>")
		set(CPACK_DEBIAN_SECTION "graphics")
		set(CPACK_DEBIAN_PACKAGE_HOMEPAGE 
		  "http://oorienteering.sourceforge.net/")
		set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")

		install(CODE "MESSAGE(\"ATTENTION: 
To build a .deb package with proper file ownership, you must run 
'fakeroot make package'.\")")
		install(
		  TARGETS Mapper 
		  DESTINATION bin)
		install(
		  FILES COPYING 
		  DESTINATION "share/doc/${CPACK_DEBIAN_PACKAGE_NAME}")
		install(
		  DIRECTORY "bin/my symbol sets/"
		  DESTINATION "share/${CPACK_DEBIAN_PACKAGE_NAME}/symbol sets"
		  FILES_MATCHING PATTERN "*.omap")
		install(
		  DIRECTORY "bin/help/"
		  DESTINATION "share/${CPACK_DEBIAN_PACKAGE_NAME}/help"
		  FILES_MATCHING PATTERN "*.qch" PATTERN "*.qhc")
		install(
		  FILES "debian/Mapper.desktop"
		  DESTINATION "share/applications")
		install(
		  FILES "debian/Mapper.xpm"
		  DESTINATION "share/pixmaps")

	endif(UNIX AND EXISTS /usr/bin/dpkg AND EXISTS /usr/bin/lsb_release)

	include(CPack)

endif(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
