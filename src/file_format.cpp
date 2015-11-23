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

// virtual
FileFormatException::~FileFormatException() noexcept
{
	// Nothing, not inlined
}

// virtual
const char* FileFormatException::what() const noexcept
{
	return msg_c.constData();
}



// ### FileFormat ###

FileFormat::FileFormat(FileFormat::FileType file_type, const QString& id, const QString& description, const QString& file_extension, FileFormat::FormatFeatures features)
 : file_type(file_type),
   format_id(id),
   format_description(description),
   format_features(features)
{
	Q_ASSERT(file_type != 0);
	Q_ASSERT(!id.isEmpty());
	Q_ASSERT(!description.isEmpty());
	Q_ASSERT(!file_extension.isEmpty());
	addExtension(file_extension);
}

FileFormat::~FileFormat()
{
	// Nothing
}

void FileFormat::addExtension(const QString& file_extension)
{
	file_extensions << file_extension;
	format_filter = QString("%1 (*.%2)").arg(format_description).arg(file_extensions.join(" *."));
}

bool FileFormat::understands(const unsigned char *buffer, size_t sz) const
{
	Q_UNUSED(buffer);
	Q_UNUSED(sz);
	return false;
}

Importer *FileFormat::createImporter(QIODevice* stream, Map *map, MapView *view) const
{
	Q_UNUSED(stream);
	Q_UNUSED(map);
	Q_UNUSED(view);
	throw FileFormatException(QString("Format (%1) does not support import").arg(description()));
}

Exporter *FileFormat::createExporter(QIODevice* stream, Map *map, MapView *view) const
{
	Q_UNUSED(stream);
	Q_UNUSED(map);
	Q_UNUSED(view);
	throw FileFormatException(QString("Format (%1) does not support export").arg(description()));
}
