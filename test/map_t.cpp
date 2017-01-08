/*
 *    Copyright 2013-2015 Kai Pastor
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

#include "core/map.h"
#include "core/map_color.h"
#include "../src/core/map_printer.h"
#include "../src/core/map_view.h"

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
	QVERIFY(Map::getCoveringRed() != NULL);
	QCOMPARE(Map::getCoveringWhite()->getPriority(), static_cast<int>(MapColor::CoveringWhite));
	
	QVERIFY(Map::getCoveringWhite() != NULL);
	QCOMPARE(Map::getCoveringRed()->getPriority(), static_cast<int>(MapColor::CoveringRed));
	
	QVERIFY(Map::getUndefinedColor() != NULL);
	QCOMPARE(Map::getUndefinedColor()->getPriority(), static_cast<int>(MapColor::Undefined));
	
	QVERIFY(Map::getRegistrationColor() != NULL);
	QCOMPARE(Map::getRegistrationColor()->getPriority(), static_cast<int>(MapColor::Registration));
	
	Map map;               // non-const
	const Map& cmap(map);  // const
	QCOMPARE(map.getMapColor(MapColor::CoveringWhite), (MapColor*)NULL);
	QCOMPARE(cmap.getMapColor(MapColor::CoveringWhite), (MapColor*)NULL);
	QCOMPARE(map.getColor(MapColor::CoveringWhite), Map::getCoveringWhite());
	QCOMPARE(cmap.getColor(MapColor::CoveringWhite), Map::getCoveringWhite());
	QCOMPARE(map.getMapColor(MapColor::CoveringRed), (MapColor*)NULL);
	QCOMPARE(cmap.getMapColor(MapColor::CoveringRed), (MapColor*)NULL);
	QCOMPARE(map.getColor(MapColor::CoveringRed), Map::getCoveringRed());
	QCOMPARE(cmap.getColor(MapColor::CoveringRed), Map::getCoveringRed());
	QCOMPARE(map.getMapColor(MapColor::Undefined), (MapColor*)NULL);
	QCOMPARE(cmap.getMapColor(MapColor::Undefined), (MapColor*)NULL);
	QCOMPARE(map.getColor(MapColor::Undefined), Map::getUndefinedColor());
	QCOMPARE(cmap.getColor(MapColor::Undefined), Map::getUndefinedColor());
	QCOMPARE(map.getMapColor(MapColor::Registration), (MapColor*)NULL);
	QCOMPARE(cmap.getMapColor(MapColor::Registration), (MapColor*)NULL);
	QCOMPARE(map.getColor(MapColor::Registration), Map::getRegistrationColor());
	QCOMPARE(cmap.getColor(MapColor::Registration), Map::getRegistrationColor());
	
	QCOMPARE(map.getMapColor(MapColor::Reserved), (MapColor*)NULL);
	QCOMPARE(cmap.getMapColor(MapColor::Reserved), (MapColor*)NULL);
	QCOMPARE(map.getColor(MapColor::Reserved), (MapColor*)NULL);
	QCOMPARE(cmap.getColor(MapColor::Reserved), (MapColor*)NULL);
	
	QCOMPARE(map.getMapColor(map.getNumColors()), (MapColor*)NULL);
	QCOMPARE(cmap.getMapColor(cmap.getNumColors()), (MapColor*)NULL);
	QCOMPARE(map.getColor(map.getNumColors()), (MapColor*)NULL);
	QCOMPARE(cmap.getColor(cmap.getNumColors()), (MapColor*)NULL);
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
#if IMPORT_MAP_DOES_NOT_USE_GUI
	Map empty_map;
	map.importMap(&empty_map, Map::CompleteImport);
	QCOMPARE(map.getNumObjects(), original_size);
	QCOMPARE(map.getNumColors(), original_num_colors);
#else
	map.color_set->importSet(*map.color_set, {});
	QCOMPARE(map.getNumColors(), original_num_colors);
#endif
	
	map.importMap(&map, Map::ColorImport);
	QCOMPARE(map.getNumObjects(), original_size);
	QCOMPARE(map.getNumColors(), original_num_colors);
	
	map.importMap(&map, Map::CompleteImport);
	QCOMPARE(map.getNumObjects(), 2*original_size);
	QCOMPARE(map.getNumColors(), original_num_colors);
	
	QString imported_path = examples_dir.absoluteFilePath(imported_file);
	Map imported_map;
	QVERIFY(imported_map.loadFrom(imported_path, nullptr, nullptr, false, false));
	
	original_size = map.getNumObjects();
	imported_map.changeScale(map.getScaleDenominator(), {}, true, true, true, true);
	map.importMap(&imported_map, Map::CompleteImport);
	QCOMPARE(map.getNumObjects(), original_size + imported_map.getNumObjects());
}

/*
 * We select a non-standard QPA because we don't need a real GUI window.
 * 
 * Normally, the "offscreen" plugin would be the correct one.
 * However, it bails out with a QFontDatabase error (cf. QTBUG-33674)
 */
auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");


QTEST_MAIN(MapTest)
