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
#include "../src/settings.h"


void SymbolSetTool::initTestCase()
{
	QCoreApplication::setOrganizationName("OpenOrienteering.org");
	QCoreApplication::setApplicationName("SymbolSetTool");
	
	doStaticInitializations();
	
	symbol_set_dir.cd(QFileInfo(__FILE__).dir().absoluteFilePath(QString("../symbol sets")));
	
	Settings::getInstance().setSetting(Settings::General_RetainCompatiblity, QVariant(false));
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
	
	QTest::newRow("ISOM 1:15000 Czech") << "ISOM_cs"  << 15000u << 15000u;
	QTest::newRow("ISOM 1:10000 Czech") << "ISOM_cs"  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000 Czech") << "ISSOM_cs" <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000 Czech") << "ISSOM_cs" <<  5000u <<  4000u;
	
	QTest::newRow("ISOM 1:15000 Finnish") << "ISOM_fi"  << 15000u << 15000u;
	QTest::newRow("ISOM 1:10000 Finnish") << "ISOM_fi"  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000 Finnish") << "ISSOM_fi" <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000 Finnish") << "ISSOM_fi" <<  5000u <<  4000u;
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
}

QTEST_MAIN(SymbolSetTool)
