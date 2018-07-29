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

#include <QString>

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
	 * Constructs a new OcdFileFormat.
	 */
	OcdFileFormat();
	
	
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
	std::unique_ptr<Exporter> makeExporter(const QString& path, const Map* map, const MapView* view) const override;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OCD_FILE_FORMAT
