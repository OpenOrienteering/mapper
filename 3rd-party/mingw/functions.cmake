#
#    Copyright 2013 Kai Pastor
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

#
# Sets the variable named by filepath to the path where the download of the
# given url will be stored or retrieved by the functions in this file.
# All downloads are stored in ${CMAKE_CURRENT_LIST_DIR}/download.
#
macro(make_filename filepath url)
	string(REGEX REPLACE ".*/" "" ${filepath} ${url})
	set(${filepath} "${PROJECT_SOURCE_DIR}/download/${${filepath}}")
endmacro()

#
# Checks that each of the named variables is set, and prints their contents.
# Raises a FATAL_ERROR if some variable is not set.
#
function(require_definitions)
	foreach(var ${ARGV})
		message("   ${var}: ${${var}}")
		if("${${var}}" STREQUAL "")
			message(FATAL_ERROR "Variable ${var} must be defined")
		endif()
	endforeach()
endfunction()

#
# Downloads the given URL if it does not exist and verifies its MD5 sum.
# Raises a FATAL_ERROR if the requested file cannot be downloaded.
#
function(download_as_needed url md5)
	make_filename(filename ${url})
	set(download_found 0)
	if(EXISTS "${filename}")
		file(MD5 "${filename}" file_md5)
		if ("${file_md5}" STREQUAL "${md5}")
			set(download_found 1)
		endif()
	endif()
	if(NOT download_found)
		message(STATUS "Downloading ${url}")
		file(
		  DOWNLOAD    "${url}"  "${filename}.tmp"
		  EXPECTED_MD5 "${md5}"
		  SHOW_PROGRESS
		)
		file(RENAME "${filename}.tmp" "${filename}")
	endif()
endfunction()

#
# Extracts the file which resulted from the previous download of the given URL
# to the named working directory.
#
function(extract_download url working_dir)
	if(NOT EXISTS "${working_dir}")
		message(FATAL_ERROR "No such directory: ${working_dir}")
	endif()
	
	make_filename(filename ${url})
	if (${filename} MATCHES "\\.7[zZ]$")
		execute_process(
		  COMMAND 7za x "${filename}"
		  WORKING_DIRECTORY "${working_dir}"
		  RESULT_VARIABLE EXTRACT_ERROR
		)
		if(EXTRACT_ERROR)
			message(FATAL_ERROR "Could not extract ${filename}")
		endif()
		if (${filename} MATCHES "\\.tar\\.7[zZ]$")
			string(REGEX REPLACE ".7[zZ]$" "" filename "${filename}")
			get_filename_component(filename "${filename}" NAME)
			execute_process(
			  COMMAND "${CMAKE_COMMAND}" -E tar xf "${filename}"
			  WORKING_DIRECTORY "${working_dir}"
			  RESULT_VARIABLE EXTRACT_ERROR
			)
			if(EXTRACT_ERROR)
				message(FATAL_ERROR "Could not extract ${filename}")
			endif()
			file(REMOVE "${working_dir}/${filename}")
		endif()
	elseif (${filename} MATCHES "\\.tar\\.[a-zA-Z]*$|\\.[zZ][iI][pP]$")
		execute_process(
		  COMMAND "${CMAKE_COMMAND}" -E tar xfz "${filename}"
		  WORKING_DIRECTORY "${working_dir}"
		  RESULT_VARIABLE EXTRACT_ERROR
		)
		if(EXTRACT_ERROR)
			message(FATAL_ERROR "Could not extract ${filename}")
		endif()
	endif()
endfunction()

macro(make_stampname name working_dir)
	set(${name} "${working_dir}/info-stamp.txt")
endmacro()

function(make_stamp working_dir url)
	if (NOT EXISTS "${working_dir}")
		message(FATAL_ERROR "No such directory: ${working_dir}")
	endif()
	make_stampname(stamp "${working_dir}")
	file(WRITE "${stamp}" "${url}")
endfunction()

function(check_stamp result working_dir url)
	set(${result} 0 PARENT_SCOPE)
	make_stampname(stamp "${working_dir}")
	if (url AND EXISTS "${stamp}")
		file(READ "${stamp}" stamp)
		if("${stamp}" STREQUAL "${url}")
			set(${result} 1 PARENT_SCOPE)
		endif()
	endif()
endfunction()
