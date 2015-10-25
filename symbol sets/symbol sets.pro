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

TEMPLATE = aux
CONFIG  -= debug_and_release

!equals(OUT_PWD, $$PWD) {
    # Copy examples to shadow build dir.
    symbol_sets.target   = symbol_sets.stamp
    symbol_sets.commands = \
      for I in 4000 5000 10000 15000; do \
        $(COPY_DIR) \"$$PWD/\$\${I}\" \"$$OUT_PWD/\"; \
      done && \
      echo > $$symbol_sets.target
    
    QMAKE_EXTRA_TARGETS += symbol_sets
    PRE_TARGETDEPS      += $$symbol_sets.target
    QMAKE_CLEAN         += $$symbol_sets.target */*.omap
}
