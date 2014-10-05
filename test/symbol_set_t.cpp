/*
 *    Copyright 2014 Kai Pastor
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

#include "symbol_set_t.h"

#include "../src/file_format_xml_p.h"
#include "../src/map.h"
#include "../src/symbol_area.h"


void SymbolSetTool::initTestCase()
{
	QCoreApplication::setOrganizationName("OpenOrienteering.org");
	QCoreApplication::setApplicationName("Mapper");
	
	doStaticInitializations();
	
	symbol_set_dir.cd(QFileInfo(__FILE__).dir().absoluteFilePath(QString("../symbol sets")));
}

void SymbolSetTool::symbolSetDirExists()
{
	QVERIFY(symbol_set_dir.exists());
}

void SymbolSetTool::processSymbolSet_data()
{
	QTest::addColumn<QString>("name");
	QTest::addColumn<unsigned int>("source_scale");
	QTest::addColumn<unsigned int>("target_scale");

	QTest::newRow("ISOM 1:15000") << "ISOM"  << 15000u << 15000u;
	QTest::newRow("ISOM 1:10000") << "ISOM"  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000") << "ISSOM" <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000") << "ISSOM" <<  5000u <<  4000u;
	
	QTest::newRow("ISOM 1:15000 Czech") << "ISOM_Čeština"  << 15000u << 15000u;
	QTest::newRow("ISOM 1:10000 Czech") << "ISOM_Čeština"  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000 Czech") << "ISSOM_Čeština" <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000 Czech") << "ISSOM_Čeština" <<  5000u <<  4000u;
	
	QTest::newRow("ISOM 1:15000 Finnish") << "ISOM_suomi"  << 15000u << 15000u;
	QTest::newRow("ISOM 1:10000 Finnish") << "ISOM_suomi"  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000 Finnish") << "ISSOM_suomi" <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000 Finnish") << "ISSOM_suomi" <<  5000u <<  4000u;
}

void SymbolSetTool::processSymbolSet()
{
	QFETCH(QString, name);
	QFETCH(unsigned int, source_scale);
	QFETCH(unsigned int, target_scale);
	
	QString source_filename = QString("src/%1_%2.xmap").arg(name).arg(source_scale);
	QVERIFY(symbol_set_dir.exists(source_filename));
	
	QString source_path = symbol_set_dir.absoluteFilePath(source_filename);
	
	Map map;
	map.loadFrom(source_path, NULL, NULL, false, false);
	
	if (source_scale != target_scale)
	{
		map.setScaleDenominator(target_scale);
		
		if (name == "ISOM")
		{
			const double factor = double(source_scale) / double(target_scale);
			map.scaleAllObjects(factor, MapCoord());
			
			int symbols_changed = 0;
			int north_lines_changed = 0;
			const int size = map.getNumSymbols();
			for (int i = 0; i < size; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				if (code < 700 && code != 602)
				{
					symbol->scale(factor);
					++symbols_changed;
				}
				
				if (code == 601 && symbol->getType() == Symbol::Area)
				{
					AreaSymbol::FillPattern& pattern0 = symbol->asArea()->getFillPattern(0);
					if (pattern0.type == AreaSymbol::FillPattern::LinePattern)
					{
						switch (target_scale)
						{
						case 10000u:
							pattern0.line_spacing = 40000;
							break;
						default:
							QFAIL("Undefined north line spacing for this scale");
						}
						++north_lines_changed;
					}
				}
			}
			QCOMPARE(symbols_changed, 139);
			QCOMPARE(north_lines_changed, 2);
		}
		
		if (name == "ISSOM")
		{
			int north_lines_changed = 0;
			const int size = map.getNumSymbols();
			for (int i = 0; i < size; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				if (code == 601 && symbol->getType() == Symbol::Area)
				{
					AreaSymbol::FillPattern& pattern0 = symbol->asArea()->getFillPattern(0);
					if (pattern0.type == AreaSymbol::FillPattern::LinePattern)
					{
						switch (target_scale)
						{
						case 4000u:
							pattern0.line_spacing = 37500;
							break;
						default:
							QFAIL("Undefined north line spacing for this scale");
						}
						++north_lines_changed;
					}
				}
			}
			QCOMPARE(north_lines_changed, 2);
		}
	}
	
	QString target_filename = QString("%2/%1_%2.omap").arg(name).arg(target_scale);
	QFile target_file(symbol_set_dir.absoluteFilePath(target_filename));
	target_file.open(QFile::WriteOnly);
	XMLFileExporter exporter(&target_file, &map, NULL);
	exporter.doExport();
	
#if 0
	// Find the file format and verify that it exists
	const FileFormat* format = FileFormats.findFormat(format_id);
	QVERIFY(format);
	
	// Load the test map
	Map* original = new Map();
	original->loadFrom(map_filename, NULL, NULL, false, false);
	QVERIFY(!original->hasUnsavedChanged());
	
	// Fix precision of grid rotation
	MapGrid grid = original->getGrid();
	grid.setAdditionalRotation(Georeferencing::roundDeclination(grid.getAdditionalRotation()));
	original->setGrid(grid);
	
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
		QFAIL(QString("Loaded map does not equal saved map, error: %1").arg(error).toLocal8Bit());
	
	comparePrinterConfig(new_map->printerConfig(), original->printerConfig());
	
	delete new_map;
	delete original;
#endif
}

#if 0
Map* SymbolSetTool::saveAndLoadMap(Map* input, const FileFormat* format)
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

bool SymbolSetTool::compareMaps(Map* a, Map* b, QString& error)
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
	
	const Georeferencing& a_geo = a->getGeoreferencing();
	const Georeferencing& b_geo = b->getGeoreferencing();
	if (a_geo.getProjectedCRSId() == "Local coordinates")
		// Fix for old native OMAP test file
		const_cast<Georeferencing&>(a_geo).setProjectedCRS("Local", a_geo.getProjectedCRSSpec());
	
	if (a_geo.isLocal() != b_geo.isLocal() ||
		a_geo.getScaleDenominator() != b_geo.getScaleDenominator() ||
		a_geo.getDeclination() != b_geo.getDeclination() ||
		a_geo.getGrivation() != b_geo.getGrivation() ||
		a_geo.getMapRefPoint() != b_geo.getMapRefPoint() ||
		a_geo.getProjectedRefPoint() != b_geo.getProjectedRefPoint() ||
		a_geo.getProjectedCRSId() != b_geo.getProjectedCRSId() ||
		a_geo.getProjectedCRSName() != b_geo.getProjectedCRSName() ||
		a_geo.getProjectedCRSSpec() != b_geo.getProjectedCRSSpec() ||
		a_geo.getGeographicRefPoint().latitude() != b_geo.getGeographicRefPoint().latitude() ||
		a_geo.getGeographicRefPoint().longitude() != b_geo.getGeographicRefPoint().longitude())
	{
		error = "The georeferencing differs.";
		return false;
	}
	
	const MapGrid& a_grid = a->getGrid();
	const MapGrid& b_grid = b->getGrid();
	
	if (a_grid != b_grid || !(a_grid == b_grid))
	{
		error = "The map grid differs.";
		return false;
	}
	
	if (a->isAreaHatchingEnabled() != b->isAreaHatchingEnabled() ||
		a->isBaselineViewEnabled() != b->isBaselineViewEnabled())
	{
		error = "The view mode differs.";
		return false;
	}
	
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
			error = QString("Color #%1 (%2) differs.").arg(i).arg(a->getColor(i)->getName());
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
	
	// Parts and objects
	// TODO: Only a single part is compared here! Add comparison for parts ...
	if (a->getNumParts() != b->getNumParts())
	{
		error = "The number of parts differs.";
		return false;
	}
	if (a->getCurrentPartIndex() != b->getCurrentPartIndex())
	{
		error = "The current part differs.";
		return false;
	}
	for (int part = 0; part < a->getNumParts(); ++part)
	{
		MapPart* a_part = a->getPart(part);
		MapPart* b_part = b->getPart(part);
		if (a_part->getName().compare(b_part->getName(), Qt::CaseSensitive) != 0)
		{
			error = QString("The names of part #%1 differ (%2 <-> %3).").arg(part).arg(a_part->getName()).arg(b_part->getName());
			return false;
		}
		if (a_part->getNumObjects() != b_part->getNumObjects())
		{
			error = "The number of objects differs.";
			return false;
		}
		for (int i = 0; i < a_part->getNumObjects(); ++i)
		{
			if (!a_part->getObject(i)->equals(b_part->getObject(i), true))
			{
				error = QString("Object #%1 (with symbol %2) in part #%3 differs.").arg(i).arg(a_part->getObject(i)->getSymbol()->getName()).arg(part);
				return false;
			}
		}
	}
	
	// Object selection
	if (a->getNumSelectedObjects() != b->getNumSelectedObjects())
	{
		error = "The number of selected objects differs.";
		return false;
	}
	if ((a->getFirstSelectedObject() == NULL && b->getFirstSelectedObject() != NULL) || (a->getFirstSelectedObject() != NULL && b->getFirstSelectedObject() == NULL) ||
		(a->getFirstSelectedObject() != NULL && b->getFirstSelectedObject() != NULL &&
		 a->getCurrentPart()->findObjectIndex(a->getFirstSelectedObject()) != b->getCurrentPart()->findObjectIndex(b->getFirstSelectedObject())))
	{
		error = "The first selected object differs.";
		return false;
	}
	for (QSet< Object* >::const_iterator it = a->selectedObjectsBegin(), end = a->selectedObjectsEnd(); it != end; ++it)
	{
		if (!b->isObjectSelected(b->getCurrentPart()->getObject(a->getCurrentPart()->findObjectIndex(*it))))
		{
			error = "The selected objects differ.";
			return false;
		}
	}
	
	// Undo steps
	// TODO: Currently only the number of steps is compared here.
	if (a->undoManager().undoStepCount() != b->undoManager().undoStepCount() ||
		a->undoManager().redoStepCount() != b->undoManager().redoStepCount())
	{
		error = "The number of undo / redo steps differs.";
		return false;
	}
	if (a->undoManager().canUndo() &&
	    a->undoManager().nextUndoStep()->getType() != b->undoManager().nextUndoStep()->getType())
	{
		error = "The type of the first undo step is different.";
		return false;
	}
	if (a->undoManager().canRedo() &&
	    a->undoManager().nextRedoStep()->getType() != b->undoManager().nextRedoStep()->getType())
	{
		error = "The type of the first redo step is different.";
		return false;
	}
	
	// Templates
	if (a->getNumTemplates() != b->getNumTemplates())
	{
		error = "The number of templates differs.";
		return false;
	}
	if (a->getFirstFrontTemplate() != b->getFirstFrontTemplate())
	{
		error = "The division into front / back templates differs.";
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

#endif

QTEST_MAIN(SymbolSetTool)
