#
#    Copyright 2019-2020 Kai Pastor
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

# Convenient access to files controlling APK packaging
add_custom_target(APK SOURCES
  AndroidManifest.xml.in
  CPackConfig.cmake.in
  install_android_package.cmake.in
  Mapper.pro.in
  strings.xml.in
)

if(NOT ANDROID)
	return()
endif()


set(MAPPER_APP_ID "default" CACHE STRING
  "Mapper Android App ID, either an explicit ID, or 'default'"
)
if(MAPPER_APP_ID STREQUAL "default" OR NOT MAPPER_APP_ID)
	# 'default' means 'org.openorienteering.mapper' plus suffix on custom version.
	# Hiding the cached setting by transient variable.
	if("${Mapper_VERSION_DISPLAY}" STREQUAL "${Mapper_VERSION}")
		set(MAPPER_APP_ID "org.openorienteering.mapper")
	else()
		set(MAPPER_APP_ID "org.openorienteering.mapper.developer")
	endif()
endif()

# The folder where to generate the qmake project. Recorded in .gitignore.
set(MAPPER_PRO_DIR "${CMAKE_CURRENT_BINARY_DIR}/Mapper")
# The folder where to stage the APK sources for the actual app ID.
set(ANDROID_PACKAGE_SOURCE_DIR "${MAPPER_PRO_DIR}/${MAPPER_APP_ID}")


if("${Qt5Core_VERSION}" VERSION_GREATER_EQUAL 5.13.0)
	set(ANDROID_MIN_SDK_VERSION 21)
elseif("${ANDROID_ABI}" MATCHES "64")
	set(ANDROID_MIN_SDK_VERSION 21)
else()
	set(ANDROID_MIN_SDK_VERSION 16)
endif()
set(ANDROID_TARGET_SDK_VERSION 28)


set(KEYSTORE_URL "KEYSTORE_URL-NOTFOUND" CACHE STRING
  "URL of the keystore to be used when signing APK packages."
)
set(KEYSTORE_ALIAS "KEYSTORE_ALIAS-NOTFOUND" CACHE STRING
  "Alias in the keystore to be used when signing APK packages."
)
if(KEYSTORE_URL AND KEYSTORE_ALIAS)
	set(SIGN_APK 1)
else()
	set(SIGN_APK 0)
endif()

if("${CMAKE_BUILD_TYPE}" MATCHES "Rel")
	set(RELEASE_APK 1)
else()
	set(RELEASE_APK 0)
endif()


if(NOT GDAL_VERSION AND EXISTS "${GDAL_LIBRARY}")
	# FIND_GDAL doesn't give a version, so let's use the library's MD5.
	file(MD5 "${GDAL_LIBRARY}" GDAL_VERSION)
endif()

file(MAKE_DIRECTORY "${MAPPER_PRO_DIR}/tmp")
configure_file(
  "CPackConfig.cmake.in"
  "${MAPPER_PRO_DIR}/tmp/CPackConfig.cmake"
  @ONLY
)
file(GENERATE
  OUTPUT "${PROJECT_BINARY_DIR}/CPackConfig.cmake"
  INPUT "${MAPPER_PRO_DIR}/tmp/CPackConfig.cmake"
)
if(NOT EXISTS "${PROJECT_BINARY_DIR}/CPackConfig.cmake")
	file(WRITE "${PROJECT_BINARY_DIR}/CPackConfig.cmake"
	  [[# Placeholder to enforce 'package' target creation]]
	)
endif()

# For Android, we create a dummy qmake project which provides
# - configuration for running androiddeployqt from CPackConfig.cmake, and
# - a project for debugging the Android app in Qt Creator
# The directory name must match the Mapper binary.
configure_file(
  "Mapper.pro.in"
  "${MAPPER_PRO_DIR}/Mapper.pro"
  @ONLY
)

configure_file(
  "AndroidManifest.xml.in"
  "${MAPPER_PRO_DIR}/tmp/AndroidManifest.xml"
  @ONLY
)
file(GENERATE
  OUTPUT "${MAPPER_PRO_DIR}/AndroidManifest.xml"
  INPUT  "${MAPPER_PRO_DIR}/tmp/AndroidManifest.xml"
)

configure_file(
  "strings.xml.in"
  "${MAPPER_PRO_DIR}/tmp/strings.xml"
  @ONLY
)

# Setup ANDROID_PACKAGE_SOURCE_DIR for qmake/androiddeployqt
get_target_property(INPUT_SOURCE_DIR Mapper_Android SOURCE_DIR)
get_target_property(INPUT_FILES Mapper_Android SOURCES)
configure_file(
  "install_android_package.cmake.in"
  "${MAPPER_PRO_DIR}/tmp/install_android_package.cmake"
  @ONLY
)
file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/install_android_package.cmake"
  INPUT  "${MAPPER_PRO_DIR}/tmp/install_android_package.cmake"
)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_android_package.cmake")
