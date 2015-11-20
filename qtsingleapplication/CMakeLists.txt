#
#    Copyright 2012 Thomas Schöps, Kai Pastor
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
 
set(QSINGLEAPP_SRCS
  qtlocalpeer.cpp
  qtsingleapplication.cpp
  qtsinglecoreapplication.cpp
)
set(QSINGLEAPP_MOC_INPUT
 qtlocalpeer.h
 qtsingleapplication.h
 qtsinglecoreapplication.h
)
qt4_wrap_cpp(QSINGLEAPP_MOC ${QSINGLEAPP_MOC_INPUT})
add_library(QtSingleApplication STATIC ${QSINGLEAPP_SRCS} ${QSINGLEAPP_MOC})

set(Mapper_LIBRARIES 
  QtSingleApplication
  ${Mapper_LIBRARIES}
  PARENT_SCOPE)

