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

namespace {

const char* idForVersion(quint16 version)
{
	switch (version)
	{
	case 1:
		return "OCD legacy";
	case 8:
		return "OCD8";
	case 9:
		return "OCD9";
	case 10:
		return "OCD10";
	case 11:
		return "OCD11";
	case 12:
		return "OCD12";
	default:
		throw FileFormatException(::OpenOrienteering::OcdFileExport::tr("Unsupported OCD version %1").arg(version));
	}
}

}  // namespace



// ### OcdFileFormat ###

OcdFileFormat::OcdFileFormat()
: FileFormat { MapFile, "OCD", ::OpenOrienteering::ImportExport::tr("OCAD, auto-detected version"), QString::fromLatin1("ocd"),
               ImportSupported | ExportSupported | ExportLossy }
{
	// Nothing
}

OcdFileFormat::OcdFileFormat(quint16 version)
: FileFormat { MapFile, idForVersion(version),
               version == 1 ? ::OpenOrienteering::ImportExport::tr("OCAD version 8, old exporter")
                            : ::OpenOrienteering::ImportExport::tr("OCAD version %1").arg(version),
               QString::fromLatin1("ocd"),
               ExportSupported | ExportLossy }
, version { version }
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


std::unique_ptr<Importer> OcdFileFormat::makeImporter(QIODevice* stream, Map *map, MapView *view) const
{
	return std::make_unique<OcdFileImport>(stream, map, view);
}

std::unique_ptr<Exporter> OcdFileFormat::makeExporter(QIODevice* stream, Map* map, MapView* view) const
{
	return std::make_unique<OcdFileExport>(stream, map, view, version);
}


}  // namespace OpenOrienteering
