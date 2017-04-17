#
#    Copyright 2017 Kai Pastor
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


# Based on Fedora 25.

include("linux-distribution.cmake")

set(explicit_copyright_gdal
  "gdal"
  "file:///usr/share/doc/gdal-libs/LICENSE.TXT"
)

set(system_copyright_dir "/usr/share")
set(copyright_pattern
  "${system_copyright_dir}/doc/@package@/COPYING"
  "${system_copyright_dir}/doc/@package@/COPYRIGHT"
  "${system_copyright_dir}/licenses/@package@/COPYING"
  "${system_copyright_dir}/licenses/@package@/Copyright.txt"
  "${system_copyright_dir}/licenses/@package@/LICENSE"
)
