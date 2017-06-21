/*
 *    Copyright 2016-2017 Kai Pastor
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

#include "fileformats/file_format.h"

class Importer;
class Map;
class MapView;
class QIODevice;


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
	 * Always returns true.
	 * 
	 * There is no cheap way to determine the answer via OGR.
	 */
	bool understands(const unsigned char *, size_t) const override;
	
	/**
	 * Creates an importer object and configures it for the given input stream
	 * and output map and view.
	 */
	Importer* createImporter(QIODevice* stream, Map *map, MapView *view) const override;
};

#endif // OPENORIENTEERING_OGR_FILE_FORMAT_H
