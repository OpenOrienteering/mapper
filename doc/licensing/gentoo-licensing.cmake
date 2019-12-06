#
#    Copyright 2019 Kai Pastor
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


include("linux-distribution.cmake")

set(system_copyright_dir "/usr/share")
# Pattern to find GDAL license.
set(copyright_pattern
  "${system_copyright_dir}/@package@/LICENSE.TXT"
)

# Location of common license text:
set(common_license_pattern "/usr/portage/licenses/@basename@")
