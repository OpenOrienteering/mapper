/*
 *    Copyright 2013, 2014, 2016-2018 Kai Pastor
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

#ifndef OPENORIENTEERING_OCD_FILE_FORMAT
#define OPENORIENTEERING_OCD_FILE_FORMAT

#include <memory>
#include <vector>

#include <QtGlobal>
#include <QString>

#include "fileformats/file_format.h"

namespace OpenOrienteering {

class Exporter;
class Importer;
class Map;
class MapView;


/**
 * The map file format known as OC*D.
 */
class OcdFileFormat : public FileFormat
{
public:
	/**
	 * Returns the file format ID string for the given version.
	 */
	static const char* idForVersion(quint16 version);
	
	/**
	 * Returns a container of all supported variants of this format.
	 */
	static std::vector<std::unique_ptr<OcdFileFormat>> makeAll();
	
	
	/**
	 * Constructs a new OcdFileFormat.
	 * 
	 * Supported explicit OCD versions for export are 8 to 12.
	 * In addition to these versions, the following special values are valid:
	 * - `autoDeterminedVersion()` supports import only. It determines the
	 *   version from the imported data.
	 * - `legacyVersion()` selects the old implementations of OCD export or
	 *   import.
	 * 
	 * Supplying an unsupported version will cause the program to abort.
	 */
	OcdFileFormat(quint16 version);
	
	
	/**
	 * Detects whether the buffer may be the start of a valid OCD file.
	 * 
	 * At the moment, it requires at least two bytes of data.
	 * It will return false if compiled for a big endian system.
	 */
	ImportSupportAssumption understands(const char* buffer, int size) const override;
	
	
	/// \copydoc FileFormat::createImporter()
	std::unique_ptr<Importer> makeImporter(const QString& path, Map* map, MapView* view) const override;
	
	/// \copydoc FileFormat::createExporter()
	std::unique_ptr<Exporter> makeExporter(const QString& path, const Map* map, const MapView* view) const override;
	
	/// The name of the property where the importer can record the imported version.
	static constexpr const char* versionProperty() { return "OcdFileFormat::version"; }
	
	/// A special value which indicates the usage of an auto-detected (or default) version.
	static constexpr quint16 autoDeterminedVersion() { return 0; }
	
	/// A special value which indicates the usage of the legacy import/export.
	static constexpr quint16 legacyVersion() { return 1; }
	
private:
	quint16 version = 0;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OCD_FILE_FORMAT
