/*
 *    Copyright 2014-2017 Kai Pastor
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

#include "settings.h"
#include "core/map.h"
#include "core/map_color.h"
#include "core/map_printer.h"
#include "core/map_view.h"
#include "core/symbols/area_symbol.h"
#include "fileformats/xml_file_format_p.h"
#include "templates/template.h"
#include "undo/undo_manager.h"


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
		existing_data.reserve(int(file.size()+1));
		existing_data = file.readAll();
		QCOMPARE(file.error(), QFileDevice::NoError);
		file.close();
		
		new_data.reserve(existing_data.size()*2);
	}
	
	QBuffer buffer(&new_data);
	buffer.open(QFile::WriteOnly);
	XMLFileExporter exporter(&buffer, map, view);
	auto is_src_format = path.contains(QLatin1String(".xmap"));
	exporter.setOption(QString::fromLatin1("autoFormatting"), is_src_format);
	auto retain_compatibility = is_src_format && map->getNumParts() == 1;
	Settings::getInstance().setSetting(Settings::General_RetainCompatiblity, retain_compatibility);
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
	
	test_data_dir.cd(QFileInfo(QString::fromUtf8(__FILE__)).dir().absoluteFilePath(QString::fromLatin1("../test/data")));
	QVERIFY(test_data_dir.exists());
	
	Template::pathForSaving = &Template::getTemplateRelativePath;
}


void SymbolSetTool::processSymbolSet_data()
{
	QTest::addColumn<QString>("name");
	QTest::addColumn<unsigned int>("source_scale");
	QTest::addColumn<unsigned int>("target_scale");

	QTest::newRow("ISOM-2000 1:15000") << QString::fromLatin1("ISOM-2000")  << 15000u << 15000u;
	QTest::newRow("ISOM-2000 1:10000") << QString::fromLatin1("ISOM-2000")  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000") << QString::fromLatin1("ISSOM") <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000") << QString::fromLatin1("ISSOM") <<  5000u <<  4000u;
	
	QTest::newRow("ISOM-2000 1:15000 Czech") << QString::fromLatin1("ISOM-2000_cs")  << 15000u << 15000u;
	QTest::newRow("ISOM-2000 1:10000 Czech") << QString::fromLatin1("ISOM-2000_cs")  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000 Czech") << QString::fromLatin1("ISSOM_cs") <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000 Czech") << QString::fromLatin1("ISSOM_cs") <<  5000u <<  4000u;
	
	QTest::newRow("ISOM-2000 1:15000 Finnish") << QString::fromLatin1("ISOM-2000_fi")  << 15000u << 15000u;
	QTest::newRow("ISOM-2000 1:10000 Finnish") << QString::fromLatin1("ISOM-2000_fi")  << 15000u << 10000u;
	QTest::newRow("ISSOM 1:5000 Finnish") << QString::fromLatin1("ISSOM_fi") <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000 Finnish") << QString::fromLatin1("ISSOM_fi") <<  5000u <<  4000u;
	
	QTest::newRow("ISSOM 1:5000 French") << QString::fromLatin1("ISSOM_fr") <<  5000u <<  5000u;
	QTest::newRow("ISSOM 1:4000 French") << QString::fromLatin1("ISSOM_fr") <<  5000u <<  4000u;
	
	QTest::newRow("ISOM-2000 1:15000 Russian") << QString::fromLatin1("ISOM-2000_ru")  << 15000u << 15000u;
	QTest::newRow("ISOM-2000 1:10000 Russian") << QString::fromLatin1("ISOM-2000_ru")  << 15000u << 10000u;
	
	QTest::newRow("ISMTBOM 1:20000") << QString::fromLatin1("ISMTBOM") << 15000u << 20000u;
	QTest::newRow("ISMTBOM 1:15000") << QString::fromLatin1("ISMTBOM") << 15000u << 15000u;
	QTest::newRow("ISMTBOM 1:10000") << QString::fromLatin1("ISMTBOM") << 15000u << 10000u;
	QTest::newRow("ISMTBOM 1:7500")  << QString::fromLatin1("ISMTBOM") << 15000u <<  7500u;
	QTest::newRow("ISMTBOM 1:5000")  << QString::fromLatin1("ISMTBOM") << 15000u <<  5000u;
	
	QTest::newRow("ISMTBOM 1:20000 Ukrainian") << QString::fromLatin1("ISMTBOM_uk") << 15000u << 20000u;
	QTest::newRow("ISMTBOM 1:15000 Ukrainian") << QString::fromLatin1("ISMTBOM_uk") << 15000u << 15000u;
	QTest::newRow("ISMTBOM 1:10000 Ukrainian") << QString::fromLatin1("ISMTBOM_uk") << 15000u << 10000u;
	QTest::newRow("ISMTBOM 1:7500 Ukrainian")  << QString::fromLatin1("ISMTBOM_uk") << 15000u <<  7500u;
	QTest::newRow("ISMTBOM 1:5000 Ukrainian")  << QString::fromLatin1("ISMTBOM_uk") << 15000u <<  5000u;
	
	QTest::newRow("ISSkiOM 1:15000") << QString::fromLatin1("ISSkiOM") << 15000u << 15000u;
	QTest::newRow("ISSkiOM 1:10000") << QString::fromLatin1("ISSkiOM") << 15000u << 10000u;
	QTest::newRow("ISSkiOM 1:5000")  << QString::fromLatin1("ISSkiOM") << 15000u <<  5000u;
	
	QTest::newRow("Course Design 1:15000") << QString::fromLatin1("Course_Design") << 10000u << 15000u;
	QTest::newRow("Course Design 1:10000") << QString::fromLatin1("Course_Design") << 10000u << 10000u;
	QTest::newRow("Course Design 1:5000")  << QString::fromLatin1("Course_Design") << 10000u <<  5000u;
	QTest::newRow("Course Design 1:4000")  << QString::fromLatin1("Course_Design") << 10000u <<  4000u;
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
	MapView view{ &map };
	map.loadFrom(source_path, nullptr, &view, false, false);
	QCOMPARE(map.getScaleDenominator(), source_scale);
	QCOMPARE(map.getNumClosedTemplates(), 0);
	
	map.resetPrinterConfig();
	map.undoManager().clear();
	for (int i = 0; i < map.getNumColors(); ++i)
	{
		auto color = map.getMapColor(i);
		if (color->getSpotColorMethod() == MapColor::CustomColor)
		{
			color->setCmykFromSpotColors();
			color->setRgbFromSpotColors();
		}
		else
		{
			color->setRgbFromCmyk();
		}
	}
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
	
	auto purple = QColor::fromCmykF(0, 1, 0, 0).hueF();
	if (source_scale != target_scale)
	{
		map.setScaleDenominator(target_scale);
		
		if (name.startsWith(QLatin1String("ISOM-2000")))
		{
			const double factor = double(source_scale) / double(target_scale);
			map.scaleAllObjects(factor, MapCoord());
			
			int symbols_changed = 0;
			int north_lines_changed = 0;
			for (int i = 0; i < num_symbols; ++i)
			{
				Symbol* symbol = map.getSymbol(i);
				const int code = symbol->getNumberComponent(0);
				const QColor& color = *symbol->guessDominantColor();
				if (qAbs(purple - color.hueF()) > 0.1
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
		else if (name.startsWith(QLatin1String("ISSOM")))
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
		else if (name.startsWith(QLatin1String("ISMTBOM")))
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
		else if (name.startsWith(QLatin1String("ISSkiOM")))
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
				const QColor& color = *symbol->guessDominantColor();
				if (qAbs(purple - color.hueF()) > 0.1
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
		else if (name.startsWith(QLatin1String("Course_Design")))
		{
			const double factor = double(source_scale) / double(target_scale);
			map.scaleAllObjects(factor, MapCoord());
		}
		else
		{
			QFAIL("Symbol set not recognized");
		}
	}
	
	MapView* new_view = nullptr;
	if (name.startsWith(QLatin1String("Course_Design")))
	{
		QCOMPARE(map.getNumTemplates(), 1);
		new_view = new MapView { &map };
		new_view->setGridVisible(true);
		if (target_scale == 10000)
			new_view->setTemplateVisibility(map.getTemplate(0), { 1, true });
		else
			map.deleteTemplate(0);
		
		auto printer_config = map.printerConfig();
		printer_config.options.show_templates = true;
		printer_config.single_page_print_area = true;
		printer_config.center_print_area = true;
		printer_config.page_format = { { 200.0, 287.0 }, 5.0 };
		printer_config.page_format.paper_size = QPrinter::A4; 
		printer_config.print_area = printer_config.page_format.page_rect;
		map.setPrinterConfig(printer_config);
	}
	else
	{
		QCOMPARE(map.getNumTemplates(), 0);
	}
	
	QString target_filename = QString::fromLatin1("%2/%1_%2.omap").arg(name, QString::number(target_scale));
	saveIfDifferent(symbol_set_dir.absoluteFilePath(target_filename), &map, new_view);
}


void SymbolSetTool::processExamples_data()
{
	QTest::addColumn<QString>("name");

	QTest::newRow("complete map")  << QString::fromLatin1("complete map");
	QTest::newRow("forest sample") << QString::fromLatin1("forest sample");
	QTest::newRow("overprinting")  << QString::fromLatin1("overprinting");
}

void SymbolSetTool::processExamples()
{
	QFETCH(QString, name);
	
	QString source_filename = QString::fromLatin1("src/%1.xmap").arg(name);
	QVERIFY(examples_dir.exists(source_filename));
	
	QString source_path = examples_dir.absoluteFilePath(source_filename);
	
	Map map;
	MapView view{ &map };
	map.loadFrom(source_path, nullptr, &view, false, false);
	
	map.undoManager().clear();
	saveIfDifferent(source_path, &map, &view);
	
	QString target_filename = QString::fromLatin1("%1.omap").arg(name);
	saveIfDifferent(examples_dir.absoluteFilePath(target_filename), &map, &view);
}


void SymbolSetTool::processTestData_data()
{
	QTest::addColumn<QString>("name");

	QTest::newRow("world-file")  << QString::fromLatin1("templates/world-file");
}

void SymbolSetTool::processTestData()
{
	QFETCH(QString, name);
	
	QString source_filename = QString::fromLatin1("%1.xmap").arg(name);
	QVERIFY(test_data_dir.exists(source_filename));
	
	QString source_path = test_data_dir.absoluteFilePath(source_filename);
	
	Map map;
	MapView view{ &map };
	map.loadFrom(source_path, nullptr, &view, false, false);
	
	map.undoManager().clear();
	saveIfDifferent(source_path, &map, &view);
}


/*
 * We don't need a real GUI window.
 * 
 * But we discovered QTBUG-58768 macOS: Crash when using QPrinter
 * while running with "minimal" platform plugin.
 */
#ifndef Q_OS_MACOS
static auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");
#endif


QTEST_MAIN(SymbolSetTool)
