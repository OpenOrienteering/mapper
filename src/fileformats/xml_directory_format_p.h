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

#ifndef OPENORIENTEERING_XML_DIRECTORY_FORMAT_P_H
#define OPENORIENTEERING_XML_DIRECTORY_FORMAT_P_H

#include <functional>
#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QSaveFile>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/symbols/symbol.h"
#include "fileformats/file_import_export.h"

namespace OpenOrienteering {


/** Map exporter for the directory based map format. */
class XMLDirectoryExporter : public Exporter
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::XMLDirectoryExporter)

public:
	XMLDirectoryExporter(const QString& path, const Map* map, const MapView* view);
	XMLDirectoryExporter() = delete;
	XMLDirectoryExporter(const XMLDirectoryExporter&) = delete;
	XMLDirectoryExporter(XMLDirectoryExporter&&) = delete;
	~XMLDirectoryExporter() override;

	XMLDirectoryExporter& operator=(const XMLDirectoryExporter&) = delete;
	XMLDirectoryExporter& operator=(XMLDirectoryExporter&&) = delete;

protected:
	bool exportImplementation() override;
	bool supportsQIODevice() const noexcept override { return false; }

	/**
	 * Creates a new member file and writes it using a writer function.
	 *
	 * Filename is the relative member-file name within the map folder
	 * (e.g. "colors.xml", "parts/index.xml"). The given function
	 * must write the root element of the member file (including its
	 * children) into the passed XML stream.
	 */
	void createMemberFile(const QString& filename, const std::function<void(QXmlStreamWriter&)>& writer);

	/** The path of the map folder, i.e. the final destination of the member files. */
	QString target_path;

	/** Files in this list will be committed when the export is done */
	std::vector<std::unique_ptr<QSaveFile>> pending_files;

private:
	/** Exports the colors to colors.xml. */
	void exportColors();

	/** Exports the symbols to symbols.xml. */
	void exportSymbols();

	/** Exports the map parts and a parts/index.xml which lists all part files. */
	void exportParts();

	/** Writes the templates to templates/index.xml. */
	void exportTemplates();

	/** Exports the printer configuration to print.xml, if any. */
	void exportPrint();

	/** Exports the view and undo/redo data to state.xml. */
	void exportState();

	/** Exports the map metadata to metadata.xml. */
	void exportMetadata();

	/** Exports the georeferencing to georeferencing.xml. */
	void exportGeoreferencing();

	/**
	 * The relative paths of the part files written to the map folder.
	 * Used to delete orphaned map part files.
	 */
	QStringList part_filenames;

};


/** Map importer for the directory based map format. */
class XMLDirectoryImporter : public Importer
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::XMLDirectoryImporter)

public:
	XMLDirectoryImporter(const QString& path, Map* map, MapView* view);
	XMLDirectoryImporter() = delete;
	XMLDirectoryImporter(const XMLDirectoryImporter&) = delete;
	XMLDirectoryImporter(XMLDirectoryImporter&&) = delete;
	~XMLDirectoryImporter() override;

	XMLDirectoryImporter& operator=(const XMLDirectoryImporter&) = delete;
	XMLDirectoryImporter& operator=(XMLDirectoryImporter&&) = delete;

protected:
	bool importImplementation() override;
	bool supportsQIODevice() const noexcept override { return false; }

	/**
	 * Reads a single member file of the map folder.
	 *
	 * The given function must consume the root element of the member file
	 * (including its children), starting at the current element.
	 * Throws FileFormatException if the file cannot be read.
	 */
	void readMemberFile(const QString& filename, const std::function<void(QXmlStreamReader&)>& reader);

private:
	/** Reads the georeferencing from georeferencing.xml, if present. */
	void importGeoreferencing();

	/** Reads the map notes and the serialization version from metadata.xml. */
	void importMetadata();

	/** Reads the colors from colors.xml. */
	void importColors();

	/** Reads the symbols from symbols.xml. */
	void importSymbols();

	/** Reads the map parts from the files listed in parts/index.xml. */
	void importParts();

	/** Reads the templates from templates/index.xml, if present. */
	void importTemplates();

	/** Reads the printer configuration from print.xml, if present. */
	void importPrint();

	/** Reads the view and undo/redo data from state.xml, if present. */
	void importState();

	/** Adds a warning for an unsupported element in the current stream. */
	void addWarningUnsupportedElement(QXmlStreamReader& xml);

	/** Verifies and fixes an inconsistent georeferencing, if necessary. */
	void validateGeoreferencing();

	/** Reads the &lt;view&gt; element of the state file. */
	void importView(QXmlStreamReader& xml);

	/** Reads the &lt;undo&gt; element of the state file. */
	void importUndo(QXmlStreamReader& xml);

	/** Reads the &lt;redo&gt; element of the state file. */
	void importRedo(QXmlStreamReader& xml);

	/** The dictionary of symbols loaded so far, used during symbol loading. */
	SymbolDictionary symbol_dict;

	/** The version of the format the member files were written with. */
	int version = -1;

	/** Whether the georeferencing was offset-adjusted on import. */
	bool georef_offset_adjusted = false;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_XML_DIRECTORY_FORMAT_P_H
