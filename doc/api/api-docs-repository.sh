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

# Usage: api-docs-repository.sh REPOSITORY BRANCH PATH

if [ ! -f repository.txt -o "$(cat repository.txt)" != "$1" ]; then
	if [ -d repository ]; then
		rm -Rf repository
	fi
	echo "$1" > repository.txt
fi

if [ ! -d repository ]; then
	git clone "$1" -b "$2" repository
	cd repository
else
	cd repository
	git checkout "$2" "${3:-.}"
	git pull --rebase
fi
