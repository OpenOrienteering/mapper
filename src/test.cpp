/*
 *    Copyright 2012 Thomas Schöps
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


#include "test.h"

#include "global.h"
#include "file_format.h"
#include "map_color.h"
#include "symbol.h"
#include "object.h"
#include "template.h"

void TestFileFormats::saveAndLoad_data()
{
	// Add all file formats which support import and export
	QTest::addColumn<QString>("format_id");
	QTest::addColumn<QString>("map_filename");
	
	const char* map_filename = "test data/COPY_OF_test_map.omap";
	Q_FOREACH(const Format* format, FileFormats.formats())
	{
		if (format->supportsExport() && format->supportsImport())
			QTest::newRow(format->id().toAscii()) << format->id() << map_filename;
	}
}

void TestFileFormats::saveAndLoad()
{
	QFETCH(QString, format_id);
	QFETCH(QString, map_filename);
	
	// Find the file format and verify that it exists
	const Format* format = FileFormats.findFormat(format_id);
	QVERIFY(format);
	
	// Load the test map
	Map* original = new Map();
	original->loadFrom(map_filename);
	
	// If the export is lossy, do one export / import cycle first to get rid of all information which cannot be exported into this format
	if (format->isExportLossy())
	{
		Map* new_map = saveAndLoadMap(original, format);
		if (!new_map)
			QFAIL("Exception while importing / exporting.");
		delete original;
		original = new_map;
	}
	
	// The test: save and load the map and compare the resulting maps
	Map* new_map = saveAndLoadMap(original, format);
	if (!new_map)
		QFAIL("Exception while importing / exporting.");
	
	QString error = "";
	bool equal = compareMaps(original, new_map, error);
	if (!equal)
		QFAIL(QString("Loaded map does not equal saved map, error: %1").arg(error).toAscii());
	
	delete new_map;
	delete original;
}

Map* TestFileFormats::saveAndLoadMap(Map* input, const Format* format)
{
	try {
		QBuffer buffer;
		buffer.open(QIODevice::ReadWrite);
		
		Exporter* exporter = format->createExporter(&buffer, input, NULL);
		if (!exporter)
			return NULL;
		exporter->doExport();
		delete exporter;
		
		buffer.seek(0);
		
		Map* out = new Map;
		Importer* importer = format->createImporter(&buffer, out, NULL);
		if (!importer)
			return NULL;
		importer->doImport(false);
		importer->finishImport();
		delete importer;
		
		buffer.close();
		
		return out;
	}
	catch (std::exception& e)
	{
		return NULL;
	}
}

bool TestFileFormats::compareMaps(Map* a, Map* b, QString& error)
{
	// TODO: This does not compare everything - yet ...
	
	// Miscellaneous
	
	if (a->getScaleDenominator() != b->getScaleDenominator())
	{
		error = "The map scales differ.";
		return false;
	}
	if (a->getMapNotes().compare(b->getMapNotes()) != 0)
	{
		error = "The map notes differ.";
		return false;
	}
	
	// TODO: print paremeters, georeferencing, etc ...
	
	// Colors
	if (a->getNumColors() != b->getNumColors())
	{
		error = "The number of colors differs.";
		return false;
	}
	for (int i = 0; i < a->getNumColors(); ++i)
	{
		if (!a->getColor(i)->equals(*b->getColor(i), true))
		{
			error = QString("Color #%1 differs.").arg(i);
			return false;
		}
	}
	
	// Symbols
	if (a->getNumSymbols() != b->getNumSymbols())
	{
		error = "The number of symbols differs.";
		return false;
	}
	for (int i = 0; i < a->getNumSymbols(); ++i)
	{
		Symbol* a_symbol = a->getSymbol(i);
		if (!a_symbol->equals(b->getSymbol(i), Qt::CaseSensitive, true))
		{
			error = QString("Symbol #%1 (%2) differs.").arg(i).arg(a_symbol->getName());
			return false;
		}
	}
	
	// Layers and objects
	// TODO: Only a single layer is compared here! Add comparison for layers ...
	if (a->getNumLayers() != b->getNumLayers())
	{
		error = "The number of layers differs.";
		return false;
	}
	for (int layer = 0; layer < a->getNumLayers(); ++layer)
	{
		MapLayer* a_layer = a->getLayer(layer);
		MapLayer* b_layer = b->getLayer(layer);
		if (a_layer->getNumObjects() != b_layer->getNumObjects())
		{
			error = "The number of objects differs.";
			return false;
		}
		for (int i = 0; i < a_layer->getNumObjects(); ++i)
		{
			if (!a_layer->getObject(i)->equals(b_layer->getObject(i), true))
			{
				error = QString("Object #%1 (with symbol %2) in layer #%3 differs.").arg(i).arg(a_layer->getObject(i)->getSymbol()->getName()).arg(layer);
				return false;
			}
		}
	}
	
	// Templates
	if (a->getNumTemplates() != b->getNumTemplates())
	{
		error = "The number of templates differs.";
		return false;
	}
	for (int i = 0; i < a->getNumTemplates(); ++i)
	{
		// TODO: only template filenames are compared
		if (a->getTemplate(i)->getTemplateFilename().compare(b->getTemplate(i)->getTemplateFilename()) != 0)
		{
			error = QString("Template filename #%1 differs.").arg(i);
			return false;
		}
	}
	
	return true;
}


int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	
	doStaticInitializations();
	
	TestFileFormats test1;
	QTest::qExec(&test1, argc, argv);
	
	return 0;
}