/*
 *    Copyright 2012, 2013 Pete Curtis, Kai Pastor
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

#ifndef _OPENORIENTEERING_FILE_FORMAT_XML_P_H
#define _OPENORIENTEERING_FILE_FORMAT_XML_P_H

#include "file_import_export.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "symbol.h"

// ### XMLFileExporter declaration ###

class XMLFileExporter : public Exporter
{
Q_OBJECT
public:
	XMLFileExporter(QIODevice* stream, Map *map, MapView *view);
	virtual ~XMLFileExporter() {}
	
	virtual void doExport() throw (FileFormatException);
	
protected:
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


// ### XMLFileImporter declaration ###

class XMLFileImporter : public Importer
{
Q_OBJECT
public:
	XMLFileImporter(QIODevice* stream, Map *map, MapView *view);
	virtual ~XMLFileImporter() {}

protected:
	virtual void import(bool load_symbols_only) throw (FileFormatException);
	
	void addWarningUnsupportedElement();
	void importGeoreferencing(bool load_symbols_only);
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
};

#endif
