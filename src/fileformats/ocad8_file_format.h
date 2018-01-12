/*
 *    Copyright 2012, 2013 Pete Curtis
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

#ifndef OPENORIENTEERING_OCAD8_FILE_FORMAT_H
#define OPENORIENTEERING_OCAD8_FILE_FORMAT_H

#include <cstddef>

#include "file_format.h"

class QIODevice;

namespace OpenOrienteering {

class Exporter;
class Importer;
class Map;
class MapView;


/** Representation of the format used by OCAD 8. 
 */
class OCAD8FileFormat : public FileFormat
{
public:
	OCAD8FileFormat();
	
	bool understands(const unsigned char *buffer, std::size_t sz) const override;
	Importer* createImporter(QIODevice* stream, Map *map, MapView *view) const override;
	Exporter* createExporter(QIODevice* stream, Map* map, MapView* view) const override;
};


}  // namespace OpenOrienteering

#endif // OCAD8_FILE_IMPORT_H
