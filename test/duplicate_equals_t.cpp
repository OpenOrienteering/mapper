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

#include <initializer_list>
#include <memory>

#include <QtGlobal>
#include <QtTest>
#include <QDir>
#include <QFileInfo>
#include <QString>

#include "global.h"
#include "test_config.h"
#include "core/map.h"
#include "core/map_part.h"
#include "core/objects/object.h"
#include "core/symbols/area_symbol.h"
#include "core/symbols/combined_symbol.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"

using namespace OpenOrienteering;


namespace
{

static const auto test_files = {
    "data:/examples/complete map.omap",
    "data:/examples/forest sample.omap",
    "data:/examples/overprinting.omap",
};


void testDecoupling(const AreaSymbol& a, const AreaSymbol& b)
{
	Q_ASSERT(&a != &b);
	for (int i = 0; i < a.getNumFillPatterns(); ++i)
	{
		auto& pattern_a = a.getFillPattern(i);
		auto& pattern_b = b.getFillPattern(i);
		QVERIFY(&a != &b);
		switch (pattern_a.type)
		{
		case AreaSymbol::FillPattern::LinePattern:
			break;
		case AreaSymbol::FillPattern::PointPattern:
			QVERIFY(pattern_a.point == nullptr || pattern_a.point != pattern_b.point);
			break;
		}
	}
}


void testDecoupling(const CombinedSymbol& a, const CombinedSymbol& b)
{
	Q_ASSERT(&a != &b);
	for (int i = 0; i < a.getNumParts(); ++i)
	{
		if (a.isPartPrivate(i))
			QVERIFY(a.getPart(i) == nullptr || a.getPart(i) != b.getPart(i));
	}
}


void testDecoupling(const LineSymbol& a, const LineSymbol& b)
{
	Q_ASSERT(&a != &b);
	QVERIFY(a.getStartSymbol() == nullptr || a.getStartSymbol() != b.getStartSymbol());
	QVERIFY(a.getMidSymbol() == nullptr || a.getMidSymbol() != b.getMidSymbol());
	QVERIFY(a.getEndSymbol() == nullptr || a.getEndSymbol() != b.getEndSymbol());
	QVERIFY(a.getDashSymbol() == nullptr || a.getDashSymbol() != b.getDashSymbol());
}


void testDecoupling(const PointSymbol& a, const PointSymbol& b)
{
	Q_ASSERT(&a != &b);
	for (int i = 0; i < a.getNumElements(); ++i)
	{
		QVERIFY(a.getElementSymbol(i) == nullptr || a.getElementSymbol(i) != b.getElementSymbol(i));
		QVERIFY(a.getElementObject(i) == nullptr || a.getElementObject(i) != b.getElementObject(i));
	}
}


}  // namespace


void DuplicateEqualsTest::initTestCase()
{
	QDir::addSearchPath(QStringLiteral("data"), QDir(QString::fromUtf8(MAPPER_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("..")));
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
	map.loadFrom(map_filename);
	
	for (int symbol = 0; symbol < map.getNumSymbols(); ++symbol)
	{
		const auto original = map.getSymbol(symbol);
		auto copy = duplicate(*original);
		switch(original->getType())
		{
		case Symbol::Area:
			QVERIFY(copy->equals(original));
			testDecoupling(static_cast<const AreaSymbol&>(*original),
			               static_cast<const AreaSymbol&>(*copy));
			break;
		case Symbol::Combined:
			QVERIFY(copy->equals(original));
			testDecoupling(static_cast<const CombinedSymbol&>(*original),
			               static_cast<const CombinedSymbol&>(*copy));
			break;
		case Symbol::Line:
			QVERIFY(copy->equals(original));
			testDecoupling(static_cast<const LineSymbol&>(*original),
			               static_cast<const LineSymbol&>(*copy));
			break;
		case Symbol::Point:
			QVERIFY(copy->equals(original));
			testDecoupling(static_cast<const PointSymbol&>(*original),
			               static_cast<const PointSymbol&>(*copy));
			break;
		case Symbol::Text:
			QVERIFY(copy->equals(original));
			break;
		default:
			Q_UNREACHABLE();
		}
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
	map.loadFrom(map_filename);
	
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
	auto Q_DECL_UNUSED qpa_selected = qputenv("QT_QPA_PLATFORM", "offscreen");  // clazy:exclude=non-pod-global-static
}
#endif


QTEST_MAIN(DuplicateEqualsTest)
