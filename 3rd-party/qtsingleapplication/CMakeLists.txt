#
#    Copyright 2012-2015 Kai Pastor
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
 
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_CLANG_TIDY "")
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "")

set(QSINGLEAPP_SRCS
  src/qtlocalpeer.cpp
  src/qtsingleapplication.cpp
  src/qtsinglecoreapplication.cpp
)

add_library(QtSingleApplication STATIC
  ${QSINGLEAPP_SRCS}
)

target_include_directories(QtSingleApplication
  SYSTEM
  INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(QtSingleApplication
  PUBLIC Qt5::Widgets
  PUBLIC Qt5::Network
)
