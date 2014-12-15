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

#ifndef OCAD8_FILE_IMPORT_H
#define OCAD8_FILE_IMPORT_H

#include "file_format.h"

/** Representation of the format used by OCAD 8. 
 */
class OCAD8FileFormat : public FileFormat
{
public:
	OCAD8FileFormat();
	
	bool understands(const unsigned char *buffer, size_t sz) const;
	virtual Importer* createImporter(QIODevice* stream, Map *map, MapView *view) const;
	virtual Exporter* createExporter(QIODevice* stream, Map* map, MapView* view) const;
};

#endif // OCAD8_FILE_IMPORT_H
