/*
 *    Copyright 2013-2017 Kai Pastor
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

#include "map_t.h"

#include <QMessageBox>

#include "core/map.h"
#include "core/map_color.h"
#include "core/map_printer.h"
#include "core/map_view.h"

namespace
{
	static QDir examples_dir;
}


void MapTest::initTestCase()
{
	Q_INIT_RESOURCE(resources);
	doStaticInitializations();
	examples_dir.cd(QFileInfo(QString::fromUtf8(__FILE__)).dir().absoluteFilePath(QString::fromLatin1("../examples")));
	// Static map initializations
	Map map;
	
	// Accept any message boxes
	connect(qApp, &QApplication::focusChanged, [](QWidget*, QWidget* w) {
		if (w && qobject_cast<QMessageBox*>(w->window()))
			QTimer::singleShot(0, w->window(), SLOT(accept()));
	});
}



void MapTest::printerConfigTest()
{
	Map map;
	QVERIFY(!map.hasPrinterConfig());
	map.setPrinterConfig({ map });
	QVERIFY(map.hasPrinterConfig());
	map.resetPrinterConfig();
	QVERIFY(!map.hasPrinterConfig());
}

void MapTest::specialColorsTest()
{
	QVERIFY(Map::getCoveringRed() != nullptr);
	QCOMPARE(Map::getCoveringWhite()->getPriority(), static_cast<int>(MapColor::CoveringWhite));
	
	QVERIFY(Map::getCoveringWhite() != nullptr);
	QCOMPARE(Map::getCoveringRed()->getPriority(), static_cast<int>(MapColor::CoveringRed));
	
	QVERIFY(Map::getUndefinedColor() != nullptr);
	QCOMPARE(Map::getUndefinedColor()->getPriority(), static_cast<int>(MapColor::Undefined));
	
	QVERIFY(Map::getRegistrationColor() != nullptr);
	QCOMPARE(Map::getRegistrationColor()->getPriority(), static_cast<int>(MapColor::Registration));
	
	Map map;               // non-const
	const Map& cmap(map);  // const
	QCOMPARE(map.getMapColor(MapColor::CoveringWhite), static_cast<MapColor*>(nullptr));
	QCOMPARE(cmap.getMapColor(MapColor::CoveringWhite), static_cast<MapColor*>(nullptr));
	QCOMPARE(map.getColor(MapColor::CoveringWhite), Map::getCoveringWhite());
	QCOMPARE(cmap.getColor(MapColor::CoveringWhite), Map::getCoveringWhite());
	QCOMPARE(map.getMapColor(MapColor::CoveringRed), static_cast<MapColor*>(nullptr));
	QCOMPARE(cmap.getMapColor(MapColor::CoveringRed), static_cast<MapColor*>(nullptr));
	QCOMPARE(map.getColor(MapColor::CoveringRed), Map::getCoveringRed());
	QCOMPARE(cmap.getColor(MapColor::CoveringRed), Map::getCoveringRed());
	QCOMPARE(map.getMapColor(MapColor::Undefined), static_cast<MapColor*>(nullptr));
	QCOMPARE(cmap.getMapColor(MapColor::Undefined), static_cast<MapColor*>(nullptr));
	QCOMPARE(map.getColor(MapColor::Undefined), Map::getUndefinedColor());
	QCOMPARE(cmap.getColor(MapColor::Undefined), Map::getUndefinedColor());
	QCOMPARE(map.getMapColor(MapColor::Registration), static_cast<MapColor*>(nullptr));
	QCOMPARE(cmap.getMapColor(MapColor::Registration), static_cast<MapColor*>(nullptr));
	QCOMPARE(map.getColor(MapColor::Registration), Map::getRegistrationColor());
	QCOMPARE(cmap.getColor(MapColor::Registration), Map::getRegistrationColor());
	
	QCOMPARE(map.getMapColor(MapColor::Reserved), static_cast<MapColor*>(nullptr));
	QCOMPARE(cmap.getMapColor(MapColor::Reserved), static_cast<MapColor*>(nullptr));
	QCOMPARE(map.getColor(MapColor::Reserved), static_cast<MapColor*>(nullptr));
	QCOMPARE(cmap.getColor(MapColor::Reserved), static_cast<MapColor*>(nullptr));
	
	QCOMPARE(map.getMapColor(map.getNumColors()), static_cast<MapColor*>(nullptr));
	QCOMPARE(cmap.getMapColor(cmap.getNumColors()), static_cast<MapColor*>(nullptr));
	QCOMPARE(map.getColor(map.getNumColors()), static_cast<MapColor*>(nullptr));
	QCOMPARE(cmap.getColor(cmap.getNumColors()), static_cast<MapColor*>(nullptr));
}

void MapTest::importTest_data()
{
	QTest::addColumn<QString>("first_file");
	QTest::addColumn<QString>("imported_file");

	QTest::newRow("complete map, overprinting")   << "complete map.omap" << "overprinting.omap";
	QTest::newRow("overprinting, forest sample")  << "overprinting.omap" << "forest sample.omap";
	QTest::newRow("forest sample, complete map")   << "forest sample.omap" << "complete map.omap";
}

void MapTest::importTest()
{
	QFETCH(QString, first_file);
	QFETCH(QString, imported_file);
	
	QString first_path = examples_dir.absoluteFilePath(first_file);
	Map map;
	MapView view{ &map };
	QVERIFY(map.loadFrom(first_path, nullptr, &view, false, false));
	
	auto original_size = map.getNumObjects();
	auto original_num_colors = map.getNumColors();
	Map empty_map;
	map.importMap(&empty_map, Map::CompleteImport);
	QCOMPARE(map.getNumObjects(), original_size);
	QCOMPARE(map.getNumColors(), original_num_colors);
	
	map.importMap(&map, Map::ColorImport);
	QCOMPARE(map.getNumObjects(), original_size);
	QCOMPARE(map.getNumColors(), original_num_colors);
	
	map.importMap(&map, Map::CompleteImport);
	QCOMPARE(map.getNumObjects(), 2*original_size);
	QCOMPARE(map.getNumColors(), original_num_colors);
	
	QString imported_path = examples_dir.absoluteFilePath(imported_file);
	Map imported_map;
	QVERIFY(imported_map.loadFrom(imported_path, nullptr, nullptr, false, false));
	QVERIFY(imported_map.getNumSymbols() > 0);
	
	original_size = map.getNumObjects();
	QHash<const Symbol*, Symbol*> symbol_map;
	map.importMap(&imported_map, Map::CompleteImport, nullptr, nullptr, -1, false, &symbol_map);
	QCOMPARE(map.getNumObjects(), original_size + imported_map.getNumObjects());
	QCOMPARE(symbol_map.size(), imported_map.getNumSymbols());
}



/*
 * We don't need a real GUI window.
 * 
 * But we discovered QTBUG-58768 macOS: Crash when using QPrinter
 * while running with "minimal" platform plugin.
 */
#ifndef Q_OS_MACOS
namespace  {
	auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");
}
#endif


QTEST_MAIN(MapTest)
