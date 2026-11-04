/*
 *    Copyright 2026 Michael Behrens
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

#ifndef OPENORIENTEERING_XML_DIRECTORY_FORMAT_H
#define OPENORIENTEERING_XML_DIRECTORY_FORMAT_H

#include <memory>

#include "fileformats/file_format.h"

class QString;

namespace OpenOrienteering {

class Exporter;
class Importer;
class Map;
class MapView;


/**
 * @brief A file format which stores a map in a folder of XML files.
 *
 * Unlike the classic single-file XML format (.omap/.xmap), this format keeps
 * the map data in a folder of separate, human-readable XML files. This makes
 * the format well suited for revision control: the map data is split by topic
 * (colors, symbols, map parts, ...) so that independent changes do not produce
 * spurious merge conflicts. Information like Undo history or current map state
 * can be ignored by ignoring the files.
 *
 * The layout of a map folder is fixed:
 * - parts/index.xml and parts/<name>.xml: the parts of the map
 * - templates/index.xml: the templates and their relative file paths
 * - colors.xml: the map colors
 * - georeferencing.xml: the map's georeferencing
 * - metadata.xml: the map metadata (notes), and the serialization version of the member files
 * - print.xml: the printer configuration, if any
 * - state.xml: the view and undo/redo state
 * - symbols.xml: the symbolset
 *
 * The folder layout itself is the reference: the member files are not
 * referenced from a manifest. The map data uses the same XML elements as the
 * classic XML format.
 *
 * Saving updates only the member files of an existing folder; all other files
 * in the folder (e.g. .gitignore files) are left alone. The templates index
 * references templates inside the map folder by their relative paths
 * (including deeper subdirectories) and templates outside the map folder by
 * their absolute paths; template files are not migrated into the map folder
 * automatically when converting a omap files into a directory.
 */
class XMLDirectoryFormat : public FileFormat
{
public:
	/** Creates a new file format of type Directory. */
	XMLDirectoryFormat();

	/** Returns true for data which can be imported from a directory. */
	ImportSupportAssumption understands(const char* buffer, int size) const override;

	/** Creates an importer for directory map files. */
	std::unique_ptr<Importer> makeImporter(const QString& path, Map* map, MapView* view) const override;

	/** Creates an exporter for directory map files. */
	std::unique_ptr<Exporter> makeExporter(const QString& path, const Map* map, const MapView* view) const override;

	/** The current directory file format version. */
	static const int current_version;
};


}  // namespace OpenOrienteering


#endif // OPENORIENTEERING_XML_DIRECTORY_FORMAT_H
