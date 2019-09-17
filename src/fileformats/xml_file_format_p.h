/*
 *    Copyright 2012, 2013 Pete Curtis
 *    Copyright 2012-2015  Kai Pastor
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

#ifndef OPENORIENTEERING_FILE_FORMAT_XML_P_H
#define OPENORIENTEERING_FILE_FORMAT_XML_P_H

#include <QCoreApplication>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/symbols/symbol.h"
#include "fileformats/file_import_export.h"

namespace OpenOrienteering {

/** Map exporter for the xml based map format. */
class XMLFileExporter : public Exporter
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::XMLFileExporter)
	
public:
	XMLFileExporter(const QString& path, const Map* map, const MapView* view);
	~XMLFileExporter() override {}
	
protected:
	bool exportImplementation() override;
	
	void exportGeoreferencing();
	void exportColors();
	void exportSymbols();
	void exportMapParts();
	void exportTemplates();
	void exportView();
	void exportPrint();
	void exportUndo();
	void exportRedo();
	
private:
	QXmlStreamWriter xml;
};


/** Map importer for the xml based map format. */
class XMLFileImporter : public Importer
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::XMLFileImporter)
	
public:
	XMLFileImporter(const QString& path, Map *map, MapView *view);
	~XMLFileImporter() override {}
	
protected:
	bool importImplementation() override;
	
	void importElements();
	
	void addWarningUnsupportedElement();
	void importMapNotes();
	void importGeoreferencing();
	void importColors();
	void importSymbols();
	void importMapParts();
	void importTemplates();
	void importView();
	void importPrint();
	void importUndo();
	void importRedo();
	
	QXmlStreamReader xml;
	SymbolDictionary symbol_dict;
	bool georef_offset_adjusted;
};


}  // namespace OpenOrienteering

#endif
