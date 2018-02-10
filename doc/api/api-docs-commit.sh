#!/bin/sh -e
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

# Usage: api-docs-commit.sh PATH [PROJECT_BINARY_DIR]

for I in "repository/${1:-.}"/*; do
	if [ -e "$I" -a "${I##*/}" != "README.md" ]; then
		rm -R "$I"
	fi
done

for I in html/*; do
	if [ "${I##*.}" != "md5" -a "${I##*.}" != "map" ]; then
		cp -R "$I" "repository/${1:-.}/"
	fi
done

cd repository
git add -A "${1:-.}"
git commit -m "Update${1:+ in $1}"

echo "Now call 'cd \"${2:-[CURRENT_BINARY_DIR]}/repository\" && git commit' to publish the updated API docs."
