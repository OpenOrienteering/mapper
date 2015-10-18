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

set(PRINTSUPPORT_SRCS
  advanced_pdf_printer.cpp
  qt-5.2.1/advanced_pdf.cpp
  qt-5.2.1/printengine_advanced_pdf.cpp
  qt-5.2.1/qfontsubset.cpp
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/qt-5.2.1")

add_library(printsupport STATIC ${PRINTSUPPORT_SRCS})

set(PRINTSUPPORT_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(PRINTSUPPORT_LIBRARIES printsupport ${ZLIB_LIBRARY} PARENT_SCOPE)

# For super-builds (e.g. cross-compilation), wait for the Qt build.
if(TARGET Qt5)
	add_dependencies(printsupport Qt5)
endif()
