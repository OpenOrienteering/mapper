/*
 *    Copyright 2013-2016 Kai Pastor
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

#include <QtGlobal>
#include <QCoreApplication>
#include <QFlags>
#include <QString>

#include "fileformats/file_import_export.h"
#include "fileformats/ocd_file_export.h"
#include "fileformats/ocd_file_import.h"


// ### OcdFileFormat ###

OcdFileFormat::OcdFileFormat()
: FileFormat { MapFile, "OCD", ImportExport::tr("OCAD"), QString::fromLatin1("ocd"),
               ImportSupported | ExportSupported | ExportLossy }
{
	// Nothing
}

bool OcdFileFormat::understands(const unsigned char* buffer, std::size_t sz) const
{
	// The first two bytes of the file must be 0x0cad in litte endian ordner.
	// This test will refuse to understand OCD files on big endian systems:
	// The importer's current implementation won't work there.
	return (sz >= 2 && *reinterpret_cast<const quint16*>(buffer) == 0x0cad);
}

Importer* OcdFileFormat::createImporter(QIODevice* stream, Map *map, MapView *view) const
{
	return new OcdFileImport(stream, map, view);
}

Exporter* OcdFileFormat::createExporter(QIODevice* stream, Map* map, MapView* view) const
{
	return new OcdFileExport(stream, map, view);
}

