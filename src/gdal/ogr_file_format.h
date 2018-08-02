/*
 *    Copyright 2016-2018 Kai Pastor
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

#ifndef OPENORIENTEERING_OGR_FILE_FORMAT_H
#define OPENORIENTEERING_OGR_FILE_FORMAT_H

#include <memory>

#include <QString>

#include "fileformats/file_format.h"

namespace OpenOrienteering {

class Importer;
class Map;
class MapView;


/**
 * A FileFormat for geospatial vector data supported by OGR.
 * 
 * Geospatial vector data cannot be loaded as a regular (OpenOrienteering) Map
 * because it has no scale. However, it typically has a spatial reference, and
 * so it can be imported into an existing map. This is the major reason for
 * implementing the OGR support as a FileFormat.
 */
class OgrFileFormat : public FileFormat
{
public:
	/**
	 * Constructs a new OgrFileFormat.
	 */
	OgrFileFormat();
	
	
	/**
	 * Creates an importer for files supported by OGR.
	 */
	std::unique_ptr<Importer> makeImporter(const QString& path, Map* map, MapView* view) const override;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_OGR_FILE_FORMAT_H
