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

# This is a toolchain file for "cross-testing" Mac OS X packaging builds from
# Linux et al. It will not cross compile Mac OS X binaries, but places Linux
# binaries in a Mac OS X bundle file system layout and creates a TGZ archive
# instead of a DMG.
#
# Synopsis:
#     cd PROJECT-DIR
#     mkdir bundle-test
#     cd bundle-test
#     cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain/bundle-test.cmake
#     cmake . -DAPPLE=1
#     make
#     make package
# 

set(CPACK_GENERATOR              TGZ  CACHE STRING "bundle-test.cmake default")
set(Mapper_BUILD_PROJ            TRUE CACHE BOOL   "bundle-test.cmake default")
set(Mapper_PACKAGE_ASSISTANT     TRUE CACHE BOOL   "bundle-test.cmake default")
set(Mapper_PACKAGE_LIBRARIES     TRUE CACHE BOOL   "bundle-test.cmake default")
set(Mapper_TRANSLATIONS_EMBEDDED TRUE CACHE BOOL   "bundle-test.cmake default")

