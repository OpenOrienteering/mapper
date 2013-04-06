/*
 *    Copyright 2013 Kai Pastor
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

#include "../src/map.h"
#include "../src/core/map_color.h"

MapTest::MapTest(QObject* parent): QObject(parent)
{
	// nothing
}

void MapTest::initTestCase()
{
	Q_INIT_RESOURCE(resources);
	doStaticInitializations();
	// Static map initializations
	Map map;
}

void MapTest::specialColorsTest()
{
	QVERIFY(Map::getCoveringRed() != NULL);
	QCOMPARE(Map::getCoveringWhite()->getPriority(), static_cast<int>(MapColor::CoveringWhite));
	
	QVERIFY(Map::getCoveringWhite() != NULL);
	QCOMPARE(Map::getCoveringRed()->getPriority(), static_cast<int>(MapColor::CoveringRed));
	
	QVERIFY(Map::getUndefinedColor() != NULL);
	QCOMPARE(Map::getUndefinedColor()->getPriority(), static_cast<int>(MapColor::Undefined));
	
	Map map;               // non-const
	const Map& cmap(map);  // const
	QCOMPARE(map.getColor(MapColor::CoveringWhite), (MapColor*)NULL);
	QCOMPARE(cmap.getColor(MapColor::CoveringWhite), Map::getCoveringWhite());
	QCOMPARE(map.getColor(MapColor::CoveringRed), (MapColor*)NULL);
	QCOMPARE(cmap.getColor(MapColor::CoveringRed), Map::getCoveringRed());
	QCOMPARE(map.getColor(MapColor::Undefined), (MapColor*)NULL);
	QCOMPARE(cmap.getColor(MapColor::Undefined), Map::getUndefinedColor());
	
	QCOMPARE(map.getColor(MapColor::Reserved), (MapColor*)NULL);
	QCOMPARE(cmap.getColor(MapColor::Reserved), (MapColor*)NULL);
	
	QCOMPARE(map.getColor(map.getNumColors()), (MapColor*)NULL);
	QCOMPARE(cmap.getColor(cmap.getNumColors()), (MapColor*)NULL);
}


QTEST_MAIN(MapTest)
