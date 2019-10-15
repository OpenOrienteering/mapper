/*
 *    Copyright 2016, 2017 Kai Pastor
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


#include <QtTest>
#include <QObject>
#include <QPointF>
#include <QRectF>

#include "core/map_coord.h"
#include "util/util.h"

using namespace OpenOrienteering;

Q_DECLARE_METATYPE(MapCoord)


/**
 * @test Tests utility functions and structures.
 */
class UtilTest : public QObject
{
Q_OBJECT
private slots:
	void initTestCase();
	void rectIncludeTest();
	void rectIncludeSafeTest();
	void pointsFormCorner_data();
	void pointsFormCorner();
};



void UtilTest::initTestCase()
{
	QRectF rect;
	
	rect = {};
	QVERIFY(!rect.isValid());
	
	rect = { 0, 0, 1, 1};
	QVERIFY(rect.isValid());
}


void UtilTest::rectIncludeTest()
{
	QRectF rect;
	
	// rectInclude on invalid rect
	rect = {};
	rectInclude(rect, QPointF{});
	QVERIFY(!rect.isValid());
	
	rect = {};
	rectInclude(rect, QRectF{});
	QVERIFY(!rect.isValid());
	
	rect = {};
	rectInclude(rect, QRectF{ 0, 0, 1, 1});
	QVERIFY(rect.isValid());
	
	
	// rectInclude on valid rect
	rect = { 0, 0, 1, 1};
	rectInclude(rect, QPointF{});
	QVERIFY(rect.isValid());
	
	rect = { 0, 0, 1, 1};
	rectInclude(rect, QRectF{});
	QVERIFY(rect.isValid());
	
	rect = { 0, 0, 1, 1};
	rectInclude(rect, QRectF{ 0, 0, 1, 1});
	QVERIFY(rect.isValid());
}

void UtilTest::rectIncludeSafeTest()
{
	QRectF rect;
	
	// rectIncludeSafe on invalid rect
	rect = {};
	rectIncludeSafe(rect, QPointF{});
	QVERIFY(rect.isValid());
	
	rect = {};
	rectIncludeSafe(rect, QRectF{});
	QVERIFY(!rect.isValid());
	
	rect = {};
	rectIncludeSafe(rect, QRectF{ 0, 0, 1, 1});
	QVERIFY(rect.isValid());
	
	
	// rectIncludeSafe on valid rect
	rect = { 0, 0, 1, 1};
	rectIncludeSafe(rect, QPointF{});
	QVERIFY(rect.isValid());
	
	rect = { 0, 0, 1, 1};
	rectIncludeSafe(rect, QRectF{});
	QVERIFY(rect.isValid());
	
	rect = { 0, 0, 1, 1};
	rectIncludeSafe(rect, QRectF{ 0, 0, 1, 1});
	QVERIFY(rect.isValid());
	
}


void UtilTest::pointsFormCorner_data()
{
	QTest::addColumn<MapCoord>("first_point");
	QTest::addColumn<MapCoord>("middle_point");
	QTest::addColumn<MapCoord>("last_point");
	QTest::addColumn<qreal>("tolerance");
	QTest::addColumn<bool>("expected_result");
	
	QTest::addRow("Straight line, zero tolerance")
	        << MapCoord { -1, 0 } << MapCoord { 0, 0 } << MapCoord { 1, 0 }
	        << 0.0 << false;	           
	QTest::addRow("Straight line, small tolerance")
	        << MapCoord { -1, 0 } << MapCoord { 0, 0 } << MapCoord { 1, 0 }
	        << 0.01 << false;	           
	QTest::addRow("Bent line, well within tolerance")
	        << MapCoord { -1, 0 } << MapCoord { 0, 0 } << MapCoord { 200, 1 }
	        << 0.01 << false;	           
	QTest::addRow("Bent line, exactly within tolerance")
	        << MapCoord { -1, 0 } << MapCoord { 0, 0 } << MapCoord { 100, 1 }
	        << 0.01 << false;	           
	QTest::addRow("Bent line, slightly outside tolerance")
	        << MapCoord { -1, 0 } << MapCoord { 0, 0 } << MapCoord { 99, 1 }
	        << 0.01 << true;
	QTest::addRow("Right angle")
	        << MapCoord { -1, 0 } << MapCoord { 0, 0 } << MapCoord { 0, 1 }
	        << 0.01 << true;
	QTest::addRow("Acute angle")
	        << MapCoord { -1, 0 } << MapCoord { 0, 0 } << MapCoord { -1, 1 }
	        << 0.01 << true;
	QTest::addRow("Zero angle")
	        << MapCoord { -1, 0 } << MapCoord { 0, 0 } << MapCoord { -1, 0 }
	        << 0.01 << true;
	QTest::addRow("Pathologic case")
	        << MapCoord { -1, 0 } << MapCoord { -1, 0 } << MapCoord { -1, 0 }
	        << 0.01 << false;
}


void UtilTest::pointsFormCorner()
{
	QFETCH(MapCoord, first_point);
	QFETCH(MapCoord, middle_point);
	QFETCH(MapCoord, last_point);
	QFETCH(qreal, tolerance);
	QFETCH(bool, expected_result);
	
	const auto result = Util::pointsFormCorner(first_point, middle_point, last_point, tolerance);
	
	QCOMPARE(result, expected_result);
}


QTEST_APPLESS_MAIN(UtilTest)
#include "util_t.moc"  // IWYU pragma: keep
