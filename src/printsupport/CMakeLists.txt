#
#    Copyright 2015 Kai Pastor
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

find_package(ZLIB REQUIRED)

if (${Qt5Core_VERSION} VERSION_LESS 5.3.0)
	set(FORK_VERSION qt-5.2.1)
elseif (${Qt5Core_VERSION} VERSION_LESS 5.5.0)
	set(FORK_VERSION qt-5.4.2)
else()
	set(FORK_VERSION qt-5.5.1)
endif()

set(PRINTSUPPORT_SRCS
  advanced_pdf_printer.cpp
  ${FORK_VERSION}/advanced_pdf.cpp
  ${FORK_VERSION}/printengine_advanced_pdf.cpp
  ${FORK_VERSION}/qfontsubset.cpp
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/${FORK_VERSION}")

add_library(printsupport STATIC ${PRINTSUPPORT_SRCS})

set(PRINTSUPPORT_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(PRINTSUPPORT_LIBRARIES printsupport ${ZLIB_LIBRARY} PARENT_SCOPE)

# For super-builds (e.g. cross-compilation), wait for the Qt build.
if(TARGET Qt5)
	add_dependencies(printsupport Qt5)
endif()
