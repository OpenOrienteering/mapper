/*
 *    Copyright 2012 Thomas Schöps, Pete Curtis
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

#include "file_format.h"

/** Provides a description of the native file format. Currently this is a (arch-dependent) binary packed format
 *  with a file extension of "omap", and internal versioning.
 */
class NativeFileFormat : public Format
{
public:
	/** Creates a new file format representing the native type.
	 */
	NativeFileFormat() : Format("native", QObject::tr("OpenOrienteering Mapper"), "omap", true, true, true) {}

	/** Returns true if the file starts with the magic byte sequence "OMAP" (0x4f 0x4d 0x41 0x50).
	 */
	bool understands(const unsigned char *buffer, size_t sz) const;

	/** Creates an importer for this file type.
	 */
	Importer *createImporter(QIODevice* stream, Map *map, MapView *view) const throw (FormatException);

	/** Creates an exporter for this file type.
	 */
	Exporter *createExporter(QIODevice* stream, Map *map, MapView *view) const throw (FormatException);

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


/** An Importer for the native file format. This class delegates to the load() and loadImpl() methods of the
 *  model objects, but long-term this can be refactored out of the model into this class.
 */
class NativeFileImport : public Importer
{
public:
	/** Creates a new native file importer.
	 */
	NativeFileImport(QIODevice* stream, Map *map, MapView *view);

	/** Destroys this importer.
	 */
	~NativeFileImport();

protected:
	/** Imports a native file.
	 */
	void import(bool load_symbols_only) throw (FormatException);
};


/** An Exporter for the native file format. This class delegates to the save() and saveImpl() methods of the
 *  model objects, but long-term this can be refactored out of the model into this class.
 */
class NativeFileExport : public Exporter
{
public:
	/** Creates a new native file exporter.
	 */
	NativeFileExport(QIODevice* stream, Map *map, MapView *view);

	/** Destroys this importer.
	 */
	~NativeFileExport();

	/** Exports a native file.
	 */
	void doExport() throw (FormatException);
};

#endif // NATIVE_FILE_FORMAT_H
