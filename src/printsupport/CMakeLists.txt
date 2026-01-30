#
#    Copyright 2015, 2016 Kai Pastor
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

find_package(Qt5Gui REQUIRED COMPONENTS Private)
find_package(Qt5PrintSupport REQUIRED COMPONENTS Private)
find_package(ZLIB REQUIRED)

set(CMAKE_CXX_CLANG_TIDY "")
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "")

if (${Qt5PrintSupport_VERSION} VERSION_LESS 5.12.0)
	message(FATAL_ERROR "Need at least Qt5PrintSupport 5.12.0, found: ${Qt5PrintSupport_VERSION}")
else()
	set(FORK_VERSION qt-5.12.4)
endif()

set(PRINTSUPPORT_SRCS
  advanced_pdf_printer.cpp
  printer_properties.h
  ${FORK_VERSION}/advanced_pdf.cpp
  ${FORK_VERSION}/printengine_advanced_pdf.cpp
  ${FORK_VERSION}/qfontsubset.cpp
)

if(WIN32)
	list(APPEND PRINTSUPPORT_SRCS printer_properties_win.cpp)
else()
	list(APPEND PRINTSUPPORT_SRCS printer_properties.cpp)
endif()

add_library(printsupport STATIC ${PRINTSUPPORT_SRCS})

target_compile_definitions(printsupport PRIVATE
  QT_NO_CAST_FROM_ASCII
  QT_NO_CAST_TO_ASCII
  QT_USE_QSTRINGBUILDER
)
target_include_directories(printsupport
  SYSTEM
  BEFORE
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${FORK_VERSION}"
  INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(printsupport
  Qt5::PrintSupport
  Qt5::Gui
  ZLIB::ZLIB
)

# Suppress warnings from Qt code
include(CheckCXXCompilerFlag)
foreach(flag IN ITEMS "-Wno-stringop-overflow")
    string(MAKE_C_IDENTIFIER "HAVE_${flag}" cache_var)
    check_cxx_compiler_flag("${flag}" "${cache_var}")
    if("${${cache_var}}")
        target_compile_options(printsupport PRIVATE "${flag}")
    endif()
endforeach()


# Workaround Qt private include dir issue
# Cf. https://bugreports.qt.io/browse/QTBUG-37417

foreach(module Core Gui PrintSupport)
	set(qt_module Qt${module})
	set(qt5_module Qt5${module})
	if("${${qt5_module}_PRIVATE_INCLUDE_DIRS}" STREQUAL "")
		foreach(base_dir ${${qt5_module}_INCLUDE_DIRS})
			if("${base_dir}" MATCHES "/${qt_module}\$")
				list(APPEND ${qt5_module}_PRIVATE_INCLUDE_DIRS "${base_dir}/${${qt5_module}_VERSION}/${qt_module}")
			endif()
		endforeach()
	endif()
	target_include_directories(printsupport SYSTEM PRIVATE ${${qt5_module}_PRIVATE_INCLUDE_DIRS})
endforeach()
