/*
 *    Copyright 2012, 2013 Pete Curtis
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

#include "file_format.h"


// ### FileFormatException ###

FileFormatException::~FileFormatException() throw()
{
	// Nothing, not inlined
}

const char* FileFormatException::what() const throw()
{
	return msg.toLocal8Bit().constData();
}



// ### FileFormat ###

FileFormat::FileFormat(const QString& id, const QString& description, const QString& file_extension, FileTypes file_types, FormatFeatures features)
 : format_id(id),
   format_description(description),
   file_extension(file_extension),
   format_filter(QString("%1 (*.%2)").arg(description).arg(file_extension)),
   file_types(file_types),
   format_features(features)
{
	// Nothing
}

FileFormat::~FileFormat()
{
	// Nothing
}

bool FileFormat::understands(const unsigned char *buffer, size_t sz) const
{
	return false;
}

Importer *FileFormat::createImporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException)
{
	throw FileFormatException(QString("Format (%1) does not support import").arg(description()));
}

Exporter *FileFormat::createExporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException)
{
	throw FileFormatException(QString("Format (%1) does not support export").arg(description()));
}
