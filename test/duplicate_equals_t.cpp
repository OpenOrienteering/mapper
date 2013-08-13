/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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

#include "duplicate_equals_t.h"

#include "../src/global.h"
#include "../src/map.h"
#include "../src/mapper_resource.h"
#include "../src/object.h"


void DuplicateEqualsTest::initTestCase()
{
	doStaticInitializations();
	map_filename = MapperResource::locate(MapperResource::TEST_DATA, "COPY_OF_test_map.omap");
	QVERIFY2(!map_filename.isEmpty(), "Unable to locate test map");
}


void DuplicateEqualsTest::symbols_data()
{
	QTest::addColumn<QString>("map_filename");
	QTest::newRow(map_filename.toLocal8Bit()) << map_filename;
}

void DuplicateEqualsTest::symbols()
{
	QFETCH(QString, map_filename);
	Map* map = new Map();
	map->loadFrom(map_filename, NULL, false, false);
	
	for (int symbol = 0; symbol < map->getNumSymbols(); ++symbol)
	{
		Symbol* original = map->getSymbol(symbol);
		Symbol* duplicate = original->duplicate();
		QVERIFY(original->equals(duplicate));
		delete duplicate;
	}
	
	delete map;
}


void DuplicateEqualsTest::objects_data()
{
	QTest::addColumn<QString>("map_filename");
	QTest::newRow(map_filename.toLocal8Bit()) << map_filename;
}

void DuplicateEqualsTest::objects()
{
	QFETCH(QString, map_filename);
	Map* map = new Map();
	map->loadFrom(map_filename, NULL, false, false);
	
	for (int part_number = 0; part_number < map->getNumParts(); ++part_number)
	{
		MapPart* part = map->getPart(part_number);
		for (int object = 0; object < part->getNumObjects(); ++object)
		{
			Object* original = part->getObject(object);
			Object* duplicate = original->duplicate();
			QVERIFY(original->equals(duplicate, true));
			delete duplicate;
		}
	}
	
	delete map;
}


QTEST_MAIN(DuplicateEqualsTest)
