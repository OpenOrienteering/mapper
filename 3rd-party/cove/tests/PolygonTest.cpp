/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 *
 * This file is part of CoVe 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PolygonTest.h"

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <limits>
#include <numeric>

#include <QtGlobal>
#include <QtMath>
#include <QtTest>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QImage>
#include <QPointF>

#include "libvectorizer/Polygons.h"

#include "test_config.h"

// Benchmarking adds significant overhead when enabled.
// With the focus on (CI) testing, we default to disabling benchmarking
// by defining COVE_BENCHMARK as an empty macro here, and let CMake set
// it to QBENCHMARK when compiling as a benchmark executable.
#ifndef COVE_BENCHMARK
#  define COVE_BENCHMARK
#endif

void PolygonTest::initTestCase()
{
	QDir::addSearchPath(QStringLiteral("testdata"), QDir(QString::fromUtf8(COVE_TEST_SOURCE_DIR)).absoluteFilePath(QStringLiteral("data")));
}

void PolygonTest::testJoins_data()
{
	QTest::addColumn<bool>("simpleOnly");
	QTest::addColumn<double>("maxDistance");
	QTest::addColumn<int>("speckleSize");
	QTest::addColumn<double>("distDirRatio");
	QTest::addColumn<QString>("imageFile");
	QTest::addColumn<QString>("resultFile");

	QTest::newRow("simple joins")
		<< true << 5.0 << 9 << 0.0 << "testdata:PolygonTest1-sample.png"
		<< "testdata:PolygonTest1-simple-joins-result.dat";
}

void PolygonTest::testJoins()
{
	cove::Polygons polyTracer;
	QImage sampleImage;

	QFETCH(bool, simpleOnly);
	QFETCH(double, maxDistance);
	QFETCH(int, speckleSize);
	QFETCH(double, distDirRatio);
	QFETCH(QString, imageFile);
	QFETCH(QString, resultFile);

	QVERIFY(sampleImage.load(imageFile));

	polyTracer.setSimpleOnly(simpleOnly);
	polyTracer.setMaxDistance(maxDistance);
	polyTracer.setSpeckleSize(speckleSize);
	polyTracer.setDistDirRatio(distDirRatio);

	cove::PolygonList polys;
	COVE_BENCHMARK
	{
		polys = polyTracer.createPolygonsFromImage(sampleImage);
	}

	// saveResults(polys, resultFile);
	compareResults(polys, resultFile);
}

void PolygonTest::saveResults(const cove::PolygonList& polys,
                              const QString& filename) const
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);

	for (auto const& poly : polys)
	{
		out << poly.isClosed();
		out << quint32(poly.size());
		for (auto const& p : poly)
			out << double(p.x()) << double(p.y());
	}
}

void PolygonTest::compareResults(const cove::PolygonList& polys,
                                 const QString& filename) const
{
	QFile file(filename);
	QVERIFY(file.open(QIODevice::ReadOnly));
	QDataStream in(&file);

	// Results may vary depending on CPU. To mitigate this issue, we count
	// deviations by distance in five classes (<= 0.5, 1.5, 2.5, 3.5, or more),
	// and compare the number of occurrences against expected maximum values.
	auto const max_errors = std::array<int, 5>{ std::numeric_limits<int>::max(), 20, 8, 4, 0 };
	auto errors = std::array<int, max_errors.size()>{};
	auto count_deviation = [&errors](double actual, double expected) {
		++*(begin(errors) + qBound(0, qCeil(qAbs(actual - expected) - 0.5), int(errors.size()) - 1));
	};
	
	for (auto const& poly : polys)
	{
		bool isClosed;
		in >> isClosed;
		QCOMPARE(poly.isClosed(), isClosed);

		quint32 polyLength;
		in >> polyLength;
		QCOMPARE(quint32(poly.size()), polyLength);

		for (auto const& p : poly)
		{
			double x, y;
			in >> x >> y;
			count_deviation(p.x(), x);
			count_deviation(p.y(), y);
		}
	}
	
	if (!std::equal(begin(errors), end(errors), begin(max_errors), std::less_equal<>()))
	{
		// Report error by displaying actual and expected distribution of deviations.
		auto as_string = [](auto const& list) {
			return std::accumulate(begin(list) + 1, end(list),
			                       QByteArray::number(list.front()).rightJustified(10),
			                       [](auto& accumulated, auto& current) {
				return accumulated + ' ' + (QByteArray::number(current).rightJustified(5));
			});
		};
		QCOMPARE(as_string(errors), as_string(max_errors));
	}
}

QTEST_GUILESS_MAIN(PolygonTest)
