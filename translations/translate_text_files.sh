#!/bin/sh
#
#    Copyright 2016 Kai Pastor
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
 
set -e

for FILE
do
	echo -n "$FILE" | sed -e 's/.*OpenOrienteering_/Comment[/;s/\.ts$/]=/'
	cat ${FILE} | sed -n -e '
	  /<source>A free software for drawing orienteering maps</! d
	  n
	  /<translation>/! n
	  s/[^>]*>//
	  s/<.*//
	  p
	  q
	'
done | sed -e "
  s/^[^=]*\]=\(Comment\)/\1/
  /^[^=]*\]=$/ d
  s/&quot;/\"/
  s/&apos;/'/
" > desktop_file_comment.txt

sed -e '
  /^Comment=/ r desktop_file_comment.txt
  /^Comment\[/ d
' -i -- "../packaging/linux/Mapper.desktop" \
&& rm -f "../packaging/linux/Mapper.desktop--"

for FILE
do
	echo -n "$FILE" | sed -e 's/.*OpenOrienteering_/     <comment xml:lang="/;s/\.ts$/">/'
	cat ${FILE} | sed -n -e '
	  /<source>Orienteering map</! d
	  n
	  /<translation>/! n
	  s/[^>]*>//
	  s/<.*//
	  p
	  q
	'
done | sed -e "
  s/^[^>]*>\( *<comment\)/\1/
  /\">$/ d
  s/$/<\/comment>/
  s/&quot;/\"/
  s/&apos;/'/
" > mime_type_comment.txt

sed -e '
  /^ *<comment>/ r mime_type_comment.txt
  /^ *<comment [^>]*lang=/ d
' -i -- "../packaging/linux/openorienteering-mapper.xml" \
&& rm -f "../packaging/linux/openorienteering-mapper.xml--"

