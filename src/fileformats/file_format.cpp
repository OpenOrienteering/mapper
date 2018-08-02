/*
 *    Copyright 2012, 2013 Pete Curtis
 *    Copyright 2018 Kai Pastor
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

#include <QCoreApplication>


namespace OpenOrienteering {

// ### FileFormatException ###

// virtual
FileFormatException::~FileFormatException() noexcept = default;

// virtual
const char* FileFormatException::what() const noexcept
{
	return msg_c.constData();
}



// ### FileFormat ###

FileFormat::FileFormat(FileFormat::FileType file_type, const char* id, const QString& description, const QString& file_extension, FileFormat::FormatFeatures features)
 : file_type(file_type),
   format_id(id),
   format_description(description),
   format_features(features)
{
	Q_ASSERT(file_type != 0);
	Q_ASSERT(qstrlen(id) > 0);
	Q_ASSERT(!description.isEmpty());
	if (!file_extension.isEmpty())
		addExtension(file_extension);
}

FileFormat::~FileFormat() = default;


void FileFormat::addExtension(const QString& file_extension)
{
	file_extensions << file_extension;
	format_filter = QString::fromLatin1("%1 (*.%2)").arg(format_description, file_extensions.join(QString::fromLatin1(" *.")));
}


FileFormat::ImportSupportAssumption FileFormat::understands(const char* /*buffer*/, int /*size*/) const
{
	return supportsImport() ? Unknown : NotSupported;
}


std::unique_ptr<Importer> FileFormat::makeImporter(const QString& /*path*/, Map* /*map*/, MapView* /*view*/) const
{
	throw FileFormatException(QCoreApplication::translate("OpenOrienteering::Importer", "Format (%1) does not support import").arg(description()));
}

std::unique_ptr<Exporter> FileFormat::makeExporter(const QString& /*path*/, const Map* /*map*/, const MapView* /*view*/) const
{
	throw FileFormatException(QCoreApplication::translate("OpenOrienteering::Exporter", "Format (%1) does not support export").arg(description()));
}


}  // namespace OpenOrienteering
