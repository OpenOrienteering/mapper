/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2018 Kai Pastor
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

#include <memory>

#include <QtTest>
#include <QFileInfo>
#include <QString>
#include <QtGlobal>

#include "global.h"
#include "mapper_resource.h"
#include "core/map.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/symbols/symbol.h"

using namespace OpenOrienteering;


namespace
{

static const auto test_files = {
    "data:/examples/complete map.omap",
    "data:/examples/forest sample.omap",
    "data:/examples/overprinting.omap",
};

}  // namespace


void DuplicateEqualsTest::initTestCase()
{
	MapperResource::setSeachPaths();
	doStaticInitializations();
	
	for (auto raw_path : test_files)
	{
		auto path = QString::fromUtf8(raw_path);
		QVERIFY(QFileInfo::exists(path));
	}
}


void DuplicateEqualsTest::symbols_data()
{
	QTest::addColumn<QString>("map_filename");
	for (auto raw_path : test_files)
	{
		QTest::newRow(raw_path) << QString::fromUtf8(raw_path);
	}
}

void DuplicateEqualsTest::symbols()
{
	QFETCH(QString, map_filename);
	Map map {};
	map.loadFrom(map_filename, nullptr, nullptr, false, false);
	
	for (int symbol = 0; symbol < map.getNumSymbols(); ++symbol)
	{
		const auto original = map.getSymbol(symbol);
		auto duplicate = std::unique_ptr<Symbol>(original->duplicate());
		QVERIFY(duplicate->equals(original));
	}
}


void DuplicateEqualsTest::objects_data()
{
	QTest::addColumn<QString>("map_filename");
	for (auto raw_path : test_files)
	{
		QTest::newRow(raw_path) << QString::fromUtf8(raw_path);
	}
}

void DuplicateEqualsTest::objects()
{
	QFETCH(QString, map_filename);
	Map map {};
	map.loadFrom(map_filename, nullptr, nullptr, false, false);
	
	for (int part_number = 0; part_number < map.getNumParts(); ++part_number)
	{
		auto part = map.getPart(part_number);
		for (int object = 0; object < part->getNumObjects(); ++object)
		{
			const auto original = part->getObject(object);
			auto duplicate = std::unique_ptr<Object>(original->duplicate());
			QVERIFY(duplicate->equals(original, true));
			QVERIFY(!duplicate->getMap());
			
			duplicate.reset(Object::getObjectForType(original->getType(), original->getSymbol()));
			QVERIFY(bool(duplicate));
			duplicate->copyFrom(*original);
			QVERIFY(duplicate->equals(original, true));
			QVERIFY(!duplicate->getMap());
		}
	}
}


/*
 * We don't need a real GUI window.
 * 
 * But we discovered QTBUG-58768 macOS: Crash when using QPrinter
 * while running with "minimal" platform plugin.
 */
#ifndef Q_OS_MACOS
namespace  {
	auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");  // clazy:exclude=non-pod-global-static
}
#endif


QTEST_MAIN(DuplicateEqualsTest)
