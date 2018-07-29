/*
 *    Copyright 2013-2016, 2018 Kai Pastor
 *
 *    Some parts taken from file_format_oc*d8{.h,_p.h,cpp} which are
 *    Copyright 2012 Pete Curtis
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

#include "ocd_file_format.h"

#include <memory>

#include <QtGlobal>
#include <QCoreApplication>
#include <QFlags>
#include <QString>

#include "fileformats/file_import_export.h"
#include "fileformats/ocd_file_export.h"
#include "fileformats/ocd_file_import.h"


namespace OpenOrienteering {

// ### OcdFileFormat ###

OcdFileFormat::OcdFileFormat()
: FileFormat { MapFile, "OCD", ::OpenOrienteering::ImportExport::tr("OCAD"), QString::fromLatin1("ocd"),
               ImportSupported | ExportSupported | ExportLossy }
{
	// Nothing
}


FileFormat::ImportSupportAssumption OcdFileFormat::understands(const char* buffer, int size) const
{
	// The first two bytes of the file must be AD 0C.
	if (size < 2)
		return Unknown;
	else if (quint8(buffer[0]) == 0xAD && buffer[1] == 0x0C)
		return FullySupported;
	else
		return NotSupported;
}


std::unique_ptr<Importer> OcdFileFormat::makeImporter(const QString& path, Map *map, MapView *view) const
{
	return std::make_unique<OcdFileImport>(path, map, view);
}

std::unique_ptr<Exporter> OcdFileFormat::makeExporter(const QString& path, const Map* map, const MapView* view) const
{
	return std::make_unique<OcdFileExport>(path, map, view);
}


}  // namespace OpenOrienteering
