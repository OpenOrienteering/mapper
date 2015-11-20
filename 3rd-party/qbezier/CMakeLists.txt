#
#    Copyright 2014 Kai Pastor
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

# qbezier_p.h and the private headers it depends on may be missing
# in some installations of Qt.
# 
# The implementation of the functions declared in qbezier_p.h are available in
# the QtGui library.
# 
# The headers need to be reviewed when switching to newer versions of Qt.
# (Last review: Qt 5.3.0 beta.)


set(QBEZIER_SRCS
  src/private/qbezier_p.h     # from qtbase/src/gui/painting/
  src/private/qdatabuffer_p.h # from qtbase/src/gui/painting/
  src/private/qmath_p.h       # from qtbase/src/gui/painting/
  src/private/qnumeric_p.h    # from qtbase/src/corelib/global/
)

add_custom_target(QBezier
  SOURCES   ${QBEZIER_SRCS}
)
