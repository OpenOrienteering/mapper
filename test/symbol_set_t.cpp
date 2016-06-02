/*
 *    Copyright 2014-2016 Kai Pastor
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

#include "../src/core/map_color.h"
#include "../src/core/map_view.h"
#include "../src/file_format_xml_p.h"
#include "../src/map.h"
#include "../src/symbol_area.h"
#include "../src/settings.h"
#include "../src/undo_manager.h"


/**
 * Saves the map to the given path iff this changes the file's content.
 */
void saveIfDifferent(const QString& path, Map* map, MapView* view = nullptr)
{
	QByteArray new_data;
	QByteArray existing_data;
	QFile file(path);
	if (file.exists())
	{
		QVERIFY(file.open(QIODevice::ReadOnly));
		existing_data.reserve(file.size()+1);
		existing_data = file.readAll();
		QCOMPARE(file.error(), QFileDevice::NoError);
		file.close();
		
		new_data.reserve(existing_data.size()*2);
	}
	
	QBuffer buffer(&new_data);
	buffer.open(QFile::WriteOnly);
	XMLFileExporter exporter(&buffer, map, view);
	auto is_src_format = bool{ path.contains(QLatin1String(".xmap")) };
	exporter.setOption(QString::fromLatin1("autoFormatting"), is_src_format);
	Settings::getInstance().setSetting(Settings::General_RetainCompatiblity, QVariant(is_src_format));
	exporter.doExport();
	QVERIFY(exporter.warnings().empty());
	buffer.close();
	
	if (new_data != existing_data)
	{
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(new_data);
		QVERIFY(file.flush());
		file.close();
		QCOMPARE(file.error(), QFileDevice::NoError);
	}
}

void SymbolSetTool::initTestCase()
{
	QCoreApplication::setOrganizationName(QString::fromLatin1("OpenOrienteering.org"));
	QCoreApplication::setApplicationName(QString::fromLatin1("SymbolSetTool"));
	
	doStaticInitializations();
	
	symbol_set_dir.cd(QFileInfo(QString::fromUtf8(__FILE__)).dir().absoluteFilePath(QString::fromLatin1("../symbol sets")));
	QVERIFY(symbol_set_dir.exists());
	
	examples_dir.cd(QFileInfo(QString::fromUtf8(__FILE__)).dir().absoluteFilePath(QString::fromLatin1("../examples")));
	QVERIFY(examples_dir.exists());
}


void SymbolSetTool::processSymbolSet_data()
{
	QTest::addColumn<QString>("name");
	QTest::addColumn<unsigned int>("source_scale");
	QTest::addColumn<unsigned int>("target_scale");

	QTest::newRow("ISOM 1:15000") << QString::fromLatin1("ISOM")  << 15000u << 15000u;
	QTest::newRow("ISOM 1:10000") << QString::fromLatin1("ISOM")  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000") << QString::fromLatin1("ISSOM") <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000") << QString::fromLatin1("ISSOM") <<  5000u <<  4000u;
	
	QTest::newRow("ISOM 1:15000 Czech") << QString::fromLatin1("ISOM_cs")  << 15000u << 15000u;
	QTest::newRow("ISOM 1:10000 Czech") << QString::fromLatin1("ISOM_cs")  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000 Czech") << QString::fromLatin1("ISSOM_cs") <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000 Czech") << QString::fromLatin1("ISSOM_cs") <<  5000u <<  4000u;
	
	QTest::newRow("ISOM 1:15000 Finnish") << QString::fromLatin1("ISOM_fi")  << 15000u << 15000u;
	QTest::newRow("ISOM 1:10000 Finnish") << QString::fromLatin1("ISOM_fi")  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000 Finnish") << QString::fromLatin1("ISSOM_fi") <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000 Finnish") << QString::fromLatin1("ISSOM_fi") <<  5000u <<  4000u;
	
	QTest::newRow("ISOM 1:15000 Russian") << QString::fromLatin1("ISOM_ru")  << 15000u << 15000u;
	QTest::newRow("ISOM 1:10000 Russian") << QString::fromLatin1("ISOM_ru")  << 15000u << 10000u;
	
	QTest::newRow("ISMTBOM 1:20000") << QString::fromLatin1("ISMTBOM") << 15000u << 20000u;
	QTest::newRow("ISMTBOM 1:15000") << QString::fromLatin1("ISMTBOM") << 15000u << 15000u;
	QTest::newRow("ISMTBOM 1:10000") << QString::fromLatin1("ISMTBOM") << 15000u << 10000u;
	QTest::newRow("ISMTBOM 1:7500")  << QString::fromLatin1("ISMTBOM") << 15000u <<  7500u;
	QTest::newRow("ISMTBOM 1:5000")  << QString::fromLatin1("ISMTBOM") << 15000u <<  5000u;
	
	QTest::newRow("ISSkiOM 1:15000") << QString::fromLatin1("ISSkiOM") << 15000u << 15000u;
	QTest::newRow("ISSkiOM 1:10000") << QString::fromLatin1("ISSkiOM") << 15000u << 10000u;
	QTest::newRow("ISSkiOM 1:5000")  << QString::fromLatin1("ISSkiOM") << 15000u <<  5000u;
}

void SymbolSetTool::processSymbolSet()
{
	QFETCH(QString, name);
	QFETCH(unsigned int, source_scale);
	QFETCH(unsigned int, target_scale);
	
	QString source_filename = QString::fromLatin1("src/%1_%2.xmap").arg(name, QString::number(source_scale));
	QVERIFY(symbol_set_dir.exists(source_filename));
	
	QString source_path = symbol_set_dir.absoluteFilePath(source_filename);
	
	Map map;
	MapView view(&map);
	map.loadFrom(source_path, nullptr, &view, false, false);
	
	map.resetPrinterConfig();
	map.undoManager().clear();
	saveIfDifferent(source_path, &map, &view);
	
	const int num_symbols = map.getNumSymbols();
	QStringList previous_numbers;
	for (int i = 0; i < num_symbols; ++i)
	{
		const Symbol* symbol = map.getSymbol(i);
		QString number = symbol->getNumberAsString();
		QString number_and_name = number + QLatin1Char(' ') % symbol->getPlainTextName();
		QVERIFY2(!symbol->getName().isEmpty(), qPrintable(number_and_name));
		QVERIFY2(!previous_numbers.contains(number), qPrintable(number_and_name));
		previous_numbers.append(number);
	}
	
	if (source_scale != target_scale)
	{
		map.setScaleDenominator(target_scale);
		
		if (name == QLatin1String("ISOM"))
		{
			const double factor = double(source_scale) / double(target_scale);
			map.scaleAllObjects(factor, MapCoord());
			
			int symbols_changed = 0;
			int north_lines_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				if (!symbol->guessDominantColor()->getSpotColorName().startsWith(QLatin1String("PURPLE"))
				    && code != 602
				    && code != 999)
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
		
		if (name == QLatin1String("ISSOM"))
		{
			int north_lines_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
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
		
		if (name == QLatin1String("ISMTBOM"))
		{
			QCOMPARE(source_scale, 15000u);
			const double factor = (target_scale >= 15000u) ? 1.0 : 1.5;
			map.scaleAllObjects(factor, MapCoord());
			
			int symbols_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				if (code != 602)
				{
					symbol->scale(factor);
					++symbols_changed;
				}
			}
			QCOMPARE(symbols_changed, 169);
		}
		
		if (name == QLatin1String("ISSkiOM"))
		{
			QCOMPARE(source_scale, 15000u);
			const double factor = (target_scale >= 15000u) ? 1.0 : 1.5;
			map.scaleAllObjects(factor, MapCoord());
			
			int symbols_changed = 0;
			int north_lines_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				if (!symbol->guessDominantColor()->getSpotColorName().startsWith(QLatin1String("PURPLE"))
				    && code != 602
				    && code != 999)
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
						case 5000u:
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
			QCOMPARE(symbols_changed, 152);
			QCOMPARE(north_lines_changed, 2);
		}
	}
	
	QString target_filename = QString::fromLatin1("%2/%1_%2.omap").arg(name, QString::number(target_scale));
	saveIfDifferent(symbol_set_dir.absoluteFilePath(target_filename), &map);
}


void SymbolSetTool::processExamples_data()
{
	QTest::addColumn<QString>("name");

	QTest::newRow("complete map")  << QString::fromLatin1("complete map");
	QTest::newRow("forest sample") << QString::fromLatin1("forest sample");
	QTest::newRow("overprinting")  << QString::fromLatin1("overprinting");
	QTest::newRow("sprint sample") << QString::fromLatin1("sprint sample");
}

void SymbolSetTool::processExamples()
{
	QFETCH(QString, name);
	
	QString source_filename = QString::fromLatin1("src/%1.xmap").arg(name);
	QVERIFY(examples_dir.exists(source_filename));
	
	QString source_path = examples_dir.absoluteFilePath(source_filename);
	
	Map map;
	MapView view(&map);
	map.loadFrom(source_path, nullptr, &view, false, false);
	
	map.undoManager().clear();
	saveIfDifferent(source_path, &map, &view);
	
	QString target_filename = QString::fromLatin1("%1.omap").arg(name);
	saveIfDifferent(examples_dir.absoluteFilePath(target_filename), &map, &view);
}


/*
 * We don't need a real GUI window.
 */
auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");


QTEST_MAIN(SymbolSetTool)
