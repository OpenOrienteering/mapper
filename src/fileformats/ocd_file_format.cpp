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
#include "fileformats/ocad8_file_format_p.h"
#include "fileformats/ocd_file_export.h"
#include "fileformats/ocd_file_import.h"


namespace OpenOrienteering {

namespace {

QString labelForVersion(quint16 version)
{
	switch (version)
	{
	case OcdFileFormat::autoDeterminedVersion():
		return ::OpenOrienteering::ImportExport::tr("OCAD");
	case OcdFileFormat::legacyVersion():
		return ::OpenOrienteering::ImportExport::tr("OCAD version 8, old implementation");
	default:
		return ::OpenOrienteering::ImportExport::tr("OCAD version %1").arg(version);
	}
}

FileFormat::Features featuresForVersion(quint16 version)
{
	switch (version)
	{
	case OcdFileFormat::legacyVersion():
		return FileFormat::Feature::FileOpen | FileFormat::Feature::FileImport | FileFormat::Feature::ReadingLossy |
		       FileFormat::Feature::FileSave | FileFormat::Feature::FileSaveAs | FileFormat::Feature::WritingLossy;
		
	case OcdFileFormat::autoDeterminedVersion():
		// Intentionally no FileFormat::ExportSupported. This prevents this
		// format from being shown in the Save-as dialog. However, it is legal
		// to create an exporter for the autoDeterminedVersion(). The actual
		// export version will be determined from the Map's versionProperty()
		// if possible, or from OcdFileExport::default_version.
		return FileFormat::Feature::FileOpen | FileFormat::Feature::FileImport | FileFormat::Feature::ReadingLossy |
		       FileFormat::Feature::FileSave | FileFormat::Feature::WritingLossy;
		
	default:
		// Intentionally no FileFormat::ImportSupported. Import is handled
		// by the autoDeterminedVersion().
		return FileFormat::Feature::FileSave | FileFormat::Feature::FileSaveAs | FileFormat::Feature::WritingLossy;
	}
}

}  // namespace



// ### OcdFileFormat ###

// static
const char* OcdFileFormat::idForVersion(quint16 version)
{
	switch (version)
	{
	case OcdFileFormat::autoDeterminedVersion():
		return "OCD";
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
	case OcdFileFormat::legacyVersion():
		return "OCD-legacy";
	default:
		qFatal("Unsupported OCD version");
	}
}


// static
std::vector<std::unique_ptr<OcdFileFormat>> OcdFileFormat::makeAll()
{
	std::vector<std::unique_ptr<OcdFileFormat>> result;
	result.reserve(7);
	result.push_back(std::make_unique<OcdFileFormat>(autoDeterminedVersion()));
	result.push_back(std::make_unique<OcdFileFormat>(12));
	result.push_back(std::make_unique<OcdFileFormat>(11));
	result.push_back(std::make_unique<OcdFileFormat>(10));
	result.push_back(std::make_unique<OcdFileFormat>(9));
	result.push_back(std::make_unique<OcdFileFormat>(8));
	result.push_back(std::make_unique<OcdFileFormat>(legacyVersion()));
	return result;
}



OcdFileFormat::OcdFileFormat(quint16 version)
: FileFormat { MapFile, idForVersion(version), labelForVersion(version), QStringLiteral("ocd"), featuresForVersion(version) }
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


std::unique_ptr<Importer> OcdFileFormat::makeImporter(const QString& path, Map *map, MapView *view) const
{
	if (version == legacyVersion())
		return std::make_unique<OCAD8FileImport>(path, map, view);
	return std::make_unique<OcdFileImport>(path, map, view);
}

std::unique_ptr<Exporter> OcdFileFormat::makeExporter(const QString& path, const Map* map, const MapView* view) const
{
	if (version == legacyVersion())
		return std::make_unique<OCAD8FileExport>(path, map, view);
	return std::make_unique<OcdFileExport>(path, map, view, version);
}


}  // namespace OpenOrienteering
