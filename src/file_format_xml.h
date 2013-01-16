/*
 *    Copyright 2012, 2013 Pete Curtis, Kai Pastor
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _OPENORIENTEERING_FILE_FORMAT_XML_H
#define _OPENORIENTEERING_FILE_FORMAT_XML_H

#include "file_format.h"

/** Interface for dealing with XML files of maps.
 */
class XMLFileFormat : public FileFormat
{
public:
	/** Creates a new file format of type XML.
	 */
	XMLFileFormat();
	
	/** Returns true if the file starts with the character sequence "<?xml".
	 *  FIXME: Needs to deal with different encodings. Provide test cases.
	 */
	bool understands(const unsigned char *buffer, size_t sz) const;
	
	/** Creates an importer for XML files.
	 */
	Importer *createImporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException);
	
	/** Creates an exporter for XML files.
	 */
	Exporter *createExporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException);
	
	/** The minimum XML file format version supported by this implementation.
	 */
	static const int minimum_version;
	
	/** The XML file format version created by this implementation.
	 */
	static const int current_version;
	
	/** The characteristic magic string at the beginning of the file
	 */
	static const QString magic_string;
	
	/** The XML namespace of the Mapper XML file format
	 */
	static const QString mapper_namespace;
};

#endif // _OPENORIENTEERING_FILE_FORMAT_XML_H
