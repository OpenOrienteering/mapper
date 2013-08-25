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


# Build configuration for x32-4.8.0-release-posix-dwarf-rev2-openorienteering

# The target MinGW configuration
set(MINGW_GCC_ARCH       x32)
set(MINGW_GCC_VERSION    4.8.0)
set(MINGW_GCC_THREADS    posix)
set(MINGW_GCC_EXCEPTIONS dwarf)
set(MINGW_GCC_REVISION   2-openorienteering)
set(MINGW_CONFIGURATION_NAME ${MINGW_GCC_ARCH}-${MINGW_GCC_VERSION}-release-${MINGW_GCC_THREADS}-${MINGW_GCC_EXCEPTIONS}-rev${MINGW_GCC_REVISION})

# The build script
set(MINGW_BUILDS_VERSION 2.0.0)
set(MINGW_BUILDS_MD5     ea7f9b929f6ff372791da9ed6796c319)
set(MINGW_SOURCES_URL    http://sourceforge.net/projects/oorienteering/files/Mapper/Source/3rd-party/mingw-builds-${MINGW_BUILDS_VERSION}.zip)

# The source package
set(MINGW_SOURCES_VERSION ${MINGW_GCC_VERSION}-release-rev${MINGW_GCC_REVISION})
set(MINGW_SOURCES_MD5     80febf5f910c1516894c70f0eaa5f21e)
set(MINGW_SOURCES_URL     http://sourceforge.net/projects/oorienteering/files/Mapper/Source/3rd-party/src-${MINGW_SOURCES_VERSION}.tar.7z)

# The MinGW toolchain
set(MINGW_TOOLCHAIN_VERSION x32-4.7.2-release-posix-sjlj-rev11)
set(MINGW_TOOLCHAIN_MD5     86241b9eb606555b4f82f7f3da56946a)
set(MINGW_TOOLCHAIN_URL     http://sourceforge.net/projects/mingwbuilds/files/host-windows/releases/4.7.3/32-bit/threads-posix/sjlj/x32-4.7.2-release-posix-sjlj-rev11.7z)

# Bootstrap a defined environment if called as a script (with cmake -P ...).
if(NOT PROJECT_NAME)
	include(${CMAKE_CURRENT_LIST_DIR}/../bootstrap.cmake)
endif()
