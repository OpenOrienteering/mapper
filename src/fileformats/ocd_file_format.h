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

#include "fileformats/file_format.h"

class QIODevice;

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
	 * Constructs a new OcdFileFormat for import and export.
	 * 
	 * The version to be exported is either detected from the map,
	 * or it is a default version. The default version can be assumed to be
	 * stable and to minimize loss of information during export and import.
	 */
	OcdFileFormat();
	
	/**
	 * Constructs a new OcdFileFormat for export to the given version of the format.
	 * 
	 * Supported OCD versions are 8 to 12.
	 * In addition to these versions, parameter value `1` selects the old
	 * implementation of OCD export.
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
	std::unique_ptr<Importer> makeImporter(QIODevice* stream, Map* map, MapView* view) const override;
	
	/// \copydoc FileFormat::createExporter()
	std::unique_ptr<Exporter> makeExporter(QIODevice* stream, Map* map, MapView* view) const override;
	
	/// The name of the property where the importer can record the imported version.
	static constexpr const char* versionProperty() { return "OcdFileFormat::version"; }
	
	/// The value of the property which indicates usage of legacy import/export.
	static constexpr quint16 legacyVersion() { return 1; }
	
private:
	quint16 version = 0;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OCD_FILE_FORMAT
