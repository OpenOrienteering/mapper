/*
 *    Copyright 2019 Kai Pastor
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

#include <utility>
#include <vector>

#include <QtMath>
#include <QtTest>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QVarLengthArray>

#include "core/map_grid.h"
#include "util/util.h"


namespace OpenOrienteering
{

namespace
{

struct Recorder
{
	struct Record { QPointF p0, p1; };
	QVarLengthArray<Record, 2> records;
	void processLine(const QPointF& a, const QPointF& b)
	{
		records.push_back({a, b});
	}
};

}  // namespace


/**
 * @test Unit test for grid operations.
 */
class GridTest : public QObject
{
Q_OBJECT
	
private slots:
	void initTestCase()
	{
		// nothing yet
	}
	
	void hatchingOperationTest()
	{
		QFETCH(double, rotation);
		QFETCH(double, offset);
		QFETCH(QPointF, p0);
		QFETCH(QPointF, p1);
		
		auto bounding_box = QRectF{ -60, -60, 120, 120 };
		auto spacing = 200.0;
		Recorder recorder;
		Util::hatchingOperation(bounding_box, spacing, offset, qDegreesToRadians(rotation), recorder);
		
		const auto num_records = int(recorder.records.size());
		QCOMPARE(num_records, 1);
		
		auto& actual = recorder.records.front();
		if (!qFuzzyCompare(actual.p0.x(), bounding_box.left()) && !qFuzzyCompare(actual.p0.y(), bounding_box.top()))
			std::swap(actual.p0, actual.p1);  // normalize
		QCOMPARE(actual.p0, p0);
		QCOMPARE(actual.p1, p1);
	}
	
	void hatchingOperationTest_data()
	{
		const auto rot37 = atan(0.75) * 180 / M_PI;
		const auto rot53 = 90 - rot37;

		QTest::addColumn<double>("rotation");
		QTest::addColumn<double>("offset");
		QTest::addColumn<QPointF>("p0");
		QTest::addColumn<QPointF>("p1");
		
		QTest::newRow("rotation:0, offset:0")     <<    0.0 <<   0.0 << QPointF{-60,  0}  << QPointF{+60,  0};
		QTest::newRow("rotation:0, offset:+1")    <<    0.0 <<  +1.0 << QPointF{-60, +1}  << QPointF{+60, +1};
		QTest::newRow("rotation:0, offset:-2")    <<    0.0 <<  -2.0 << QPointF{-60, -2}  << QPointF{+60, -2};
		QTest::newRow("rotation:+360, offset:-2") << +360.0 <<  -2.0 << QPointF{-60, -2}  << QPointF{+60, -2};
		QTest::newRow("rotation:-360, offset:-2") << -360.0 <<  -2.0 << QPointF{-60, -2}  << QPointF{+60, -2};
		
		QTest::newRow("rotation:+37, offset:0")   << +rot37 <<   0.0 << QPointF{-60, +45} << QPointF{+60, -45};
		QTest::newRow("rotation:+37, offset:+12") << +rot37 << +12.0 << QPointF{-60, +60} << QPointF{+60, -30};
		QTest::newRow("rotation:+37, offset:-12") << +rot37 << -12.0 << QPointF{+60, -60} << QPointF{-60, +30};
		QTest::newRow("rotation:+37, offset:+24") << +rot37 << +24.0 << QPointF{-40, +60} << QPointF{+60, -15};
		QTest::newRow("rotation:+37, offset:-24") << +rot37 << -24.0 << QPointF{+40, -60} << QPointF{-60, +15};
		
		QTest::newRow("rotation:+53, offset:0")   << +rot53 <<   0.0 << QPointF{+45, -60} << QPointF{-45, +60};
		QTest::newRow("rotation:+53, offset:+12") << +rot53 << +12.0 << QPointF{+60, -60} << QPointF{-30, +60};
		QTest::newRow("rotation:+53, offset:-12") << +rot53 << -12.0 << QPointF{+30, -60} << QPointF{-60, +60};
		QTest::newRow("rotation:+53, offset:+24") << +rot53 << +24.0 << QPointF{-15, +60} << QPointF{+60, -40};
		QTest::newRow("rotation:+53, offset:-24") << +rot53 << -24.0 << QPointF{+15, -60} << QPointF{-60, +40};
		
		QTest::newRow("rotation:+90, offset:0")   <<  +90.0 <<   0.0 << QPointF{ 0, -60}  << QPointF{ 0, +60};
		QTest::newRow("rotation:+90, offset:+1")  <<  +90.0 <<  +1.0 << QPointF{+1, -60}  << QPointF{+1, +60};
		QTest::newRow("rotation:+90, offset:-2")  <<  +90.0 <<  -2.0 << QPointF{-2, -60}  << QPointF{-2, +60};
		QTest::newRow("rotation:-270, offset:-2") << -270.0 <<  -2.0 << QPointF{-2, -60}  << QPointF{-2, +60};
		
		QTest::newRow("rotation:+180, offset:0")  << +180.0 <<  +0.0 << QPointF{-60,  0}  << QPointF{+60,  0};
		QTest::newRow("rotation:+180, offset:+1") << +180.0 <<  +1.0 << QPointF{-60, -1}  << QPointF{+60, -1};
		QTest::newRow("rotation:+179.9999, offset:+1") << +179.9999 << +1.0 << QPointF{-60, -1}  << QPointF{+60, -1};
		QTest::newRow("rotation:+180, offset:-2") << +180.0 <<  -2.0 << QPointF{-60, +2}  << QPointF{+60, +2};
		QTest::newRow("rotation:-180, offset:-2") << -180.0 <<  -2.0 << QPointF{-60, +2}  << QPointF{+60, +2};
		
		QTest::newRow("rotation:+270, offset:0")  << +270.0 <<   0.0 << QPointF{ 0, -60}  << QPointF{ 0, +60};
		QTest::newRow("rotation:+270, offset:+1") << +270.0 <<  +1.0 << QPointF{-1, -60}  << QPointF{-1, +60};
		QTest::newRow("rotation:+270, offset:-2") << +270.0 <<  -2.0 << QPointF{+2, -60}  << QPointF{+2, +60};
		QTest::newRow("rotation:-90, offset:-2")  <<  -90.0 <<  -2.0 << QPointF{+2, -60}  << QPointF{+2, +60};
		
		QTest::newRow("rotation:-53, offset:0")   << -rot53 <<   0.0 << QPointF{-45, -60} << QPointF{+45, +60};
		QTest::newRow("rotation:-53, offset:+12") << -rot53 << +12.0 << QPointF{-60, -60} << QPointF{+30, +60};
		QTest::newRow("rotation:-53, offset:-12") << -rot53 << -12.0 << QPointF{-30, -60} << QPointF{+60, +60};
		QTest::newRow("rotation:-53, offset:+24") << -rot53 << +24.0 << QPointF{-60, -40} << QPointF{+15, +60};
		QTest::newRow("rotation:-53, offset:-24") << -rot53 << -24.0 << QPointF{-15, -60} << QPointF{+60, +40};
		
		QTest::newRow("rotation:-37, offset:0")   << -rot37 <<   0.0 << QPointF{-60, -45} << QPointF{+60, +45};
		QTest::newRow("rotation:-37, offset:+12") << -rot37 << +12.0 << QPointF{-60, -30} << QPointF{+60, +60};
		QTest::newRow("rotation:-37, offset:-12") << -rot37 << -12.0 << QPointF{-60, -60} << QPointF{+60, +30};
		QTest::newRow("rotation:-37, offset:+24") << -rot37 << +24.0 << QPointF{-60, -15} << QPointF{+40, +60};
		QTest::newRow("rotation:-37, offset:-24") << -rot37 << -24.0 << QPointF{-40, -60} << QPointF{+60, +15};
	}
	
	
	
	void gridOperationTest()
	{
		QFETCH(double, rotation);
		QFETCH(double, offset);
		QFETCH(QPointF, p00);
		QFETCH(QPointF, p01);
		QFETCH(QPointF, p10);
		QFETCH(QPointF, p11);
		
		auto bounding_box = QRectF{ -5, -5, 10, 10 };
		auto spacing = 100.0;
		Recorder recorder;
		Util::gridOperation(bounding_box, spacing, spacing, offset, offset, qDegreesToRadians(rotation), recorder);
		
		const auto num_records = int(recorder.records.size());
		QCOMPARE(num_records, 2);
		
		if (!qIsNull(recorder.records.front().p0.y())
		    && qFuzzyCompare(recorder.records.front().p0.y(), -recorder.records.front().p1.y())
		    && !qIsNull(recorder.records.back().p0.x())
		    && qFuzzyCompare(recorder.records.back().p0.x(), -recorder.records.back().p1.x()))
		{
			std::swap(recorder.records.front(), recorder.records.back());  // normalize
		}
		
		{
			auto& actual = recorder.records.front();
			if (!qFuzzyCompare(actual.p0.x(), bounding_box.left()) && !qFuzzyCompare(actual.p0.y(), bounding_box.top()))
				std::swap(actual.p0, actual.p1);  // normalize
			QCOMPARE(actual.p0, p00);
			QCOMPARE(actual.p1, p01);
		}
		
		{
			auto& actual = recorder.records.back();
			if (!qFuzzyCompare(actual.p0.x(), bounding_box.left()) && !qFuzzyCompare(actual.p0.y(), bounding_box.top()))
				std::swap(actual.p0, actual.p1);  // normalize
			QCOMPARE(actual.p0, p10);
			QCOMPARE(actual.p1, p11);
		}
	}
	
	void gridOperationTest_data()
	{
		QTest::addColumn<double>("rotation");
		QTest::addColumn<double>("offset");
		QTest::addColumn<QPointF>("p00");
		QTest::addColumn<QPointF>("p01");
		QTest::addColumn<QPointF>("p10");
		QTest::addColumn<QPointF>("p11");
		
		QTest::newRow("rotation:0, offset:0")     <<    0.0 <<  0.0 << QPointF{-5,  0} << QPointF{+5,  0} << QPointF{ 0, -5} << QPointF{ 0, +5};
		QTest::newRow("rotation:0, offset:+1")    <<    0.0 << +1.0 << QPointF{-5, +1} << QPointF{+5, +1} << QPointF{-1, -5} << QPointF{-1, +5};
		QTest::newRow("rotation:+360, offset:+1") << +360.0 << +1.0 << QPointF{-5, +1} << QPointF{+5, +1} << QPointF{-1, -5} << QPointF{-1, +5};
		QTest::newRow("rotation:-360, offset:+1") << -360.0 << +1.0 << QPointF{-5, +1} << QPointF{+5, +1} << QPointF{-1, -5} << QPointF{-1, +5};
		
		QTest::newRow("rotation:+90, offset:+1")  <<  +90.0 << +1.0 << QPointF{-5, +1} << QPointF{+5, +1} << QPointF{+1, -5} << QPointF{+1, +5};
		QTest::newRow("rotation:-270, offset:+1") << -270.0 << +1.0 << QPointF{-5, +1} << QPointF{+5, +1} << QPointF{+1, -5} << QPointF{+1, +5};
		
		QTest::newRow("rotation:+180, offset:+1") << +180.0 << +1.0 << QPointF{-5, -1} << QPointF{+5, -1} << QPointF{+1, -5} << QPointF{+1, +5};
		QTest::newRow("rotation:-180, offset:+1") << -180.0 << +1.0 << QPointF{-5, -1} << QPointF{+5, -1} << QPointF{+1, -5} << QPointF{+1, +5};
		
		QTest::newRow("rotation:+270, offset:+1") << +270.0 << +1.0 << QPointF{-5, -1} << QPointF{+5, -1} << QPointF{-1, -5} << QPointF{-1, +5};
		QTest::newRow("rotation:-90, offset:+1")  <<  -90.0 << +1.0 << QPointF{-5, -1} << QPointF{+5, -1} << QPointF{-1, -5} << QPointF{-1, +5};
	}
	
};  // class GridTest


}  // namespace OpenOrienteering



QTEST_APPLESS_MAIN(OpenOrienteering::GridTest)

#include "grid_t.moc"  // IWYU pragma: keep
