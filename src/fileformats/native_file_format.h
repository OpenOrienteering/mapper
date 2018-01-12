/*
 *    Copyright 2012, 2013 Thomas Schöps, Pete Curtis
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

#ifndef NATIVE_FILE_FORMAT_H
#define NATIVE_FILE_FORMAT_H

#ifndef NO_NATIVE_FILE_FORMAT

#include <cstddef>

#include "fileformats/file_format.h"

class QIODevice;

class Importer;

namespace OpenOrienteering {

class Map;
class MapView;


/** Provides a description of the old native file format. 
 *  This is an (architecture-dependent) binary packed format
 *  with a file extension of "omap", and internal versioning.
 * 
 *  \deprecated
 */
class NativeFileFormat : public FileFormat
{
public:
	/** Creates a new file format representing the native type.
	 */
	NativeFileFormat();
	
	/** Returns true if the file starts with the magic byte sequence "OMAP" (0x4f 0x4d 0x41 0x50).
	 */
	bool understands(const unsigned char *buffer, std::size_t sz) const override;
	
	/** Creates an importer for this file type.
	 */
	Importer *createImporter(QIODevice* stream, Map *map, MapView *view) const override;
	
#ifdef MAPPER_ENABLE_NATIVE_EXPORTER
	/** Creates an exporter for this file type.
	 */
	Exporter *createExporter(QIODevice* stream, Map *map, MapView *view) const;
#endif
	
	/** Constant describing the earliest OMAP version supported by this file format.
	 */
	static const int least_supported_file_format_version;
	
	/** Constant describing the current OMAP version used by this file format for saving.
	 */
	static const int current_file_format_version;
	
	/** The file magic: "OMAP"
	 */
	static const char magic_bytes[4];
};

#endif // NO_NATIVE_FILE_FORMAT


}  // namespace OpenOrienteering

#endif // NATIVE_FILE_FORMAT_H
