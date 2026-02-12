#
#    Copyright 2026 Kai Pastor
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


if(NOT VCPKG_INSTALLED_DIR)
	message(FATAL_ERROR "VCPKG_INSTALLED_DIR is not set")
elseif(NOT VCPKG_TARGET_TRIPLET)
	message(FATAL_ERROR "VCPKG_TARGET_TRIPLET is not set")
endif()

file(GLOB coypright_files "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/*/copyright")
foreach(file IN LISTS coypright_files)
  get_filename_component(package_dir "${file}" DIRECTORY)
  get_filename_component(package_name "${package_dir}" NAME)
  list(APPEND third_party_components "${package_name}")
endforeach()

# vcpkg packages which do not become part of the artifacts
list(REMOVE_ITEM third_party_components
  gettext
  glib
  pkgconf
  vcpkg-boost
  vcpkg-cmake
  vcpkg-cmake-config
  vcpkg-cmake-get-vars
  vcpkg-get-python-packages
  vcpkg-make
  vcpkg-pkgconfig-get-modules
  vcpkg-tool-meson
)

# Map component names to source package names
set(package_names
  libpolyclipping:polyclipping
  qtandroidextras:qt5-androidextras
  qtbase:qt5-base
  qtimageformats:qt5-imageformats
  qtlocation:qt5-location
  qtsensors:qt5-sensors
  qtserialport:qt5-serialport
  qttools:qt5-tools
  qttranslations:qt5-translations
)

set(copyright_pattern "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/@package@/copyright")
