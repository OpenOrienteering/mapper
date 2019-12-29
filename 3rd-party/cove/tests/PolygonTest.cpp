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

#include <QDataStream>
#include <QFile>
#include <QIODevice>
#include <QImage>
#include <QtGlobal>
#include <QtTest>

#include "libvectorizer/Polygons.h"

// Benchmarking adds significant overhead when enabled.
// With the focus on (CI) testing, we default to disabling benchmarking
// by defining COVE_BENCHMARK as an empty macro here, and let CMake set
// it to QBENCHMARK when the user chooses to enable COVE_BENCHMARKING
// at configuration time.
#ifndef COVE_BENCHMARK
#  define COVE_BENCHMARK
#endif

void PolygonTest::testJoins_data()
{
	QTest::addColumn<bool>("simpleOnly");
	QTest::addColumn<double>("maxDistance");
	QTest::addColumn<int>("speckleSize");
	QTest::addColumn<double>("distDirRatio");
	QTest::addColumn<QString>("imageFile");
	QTest::addColumn<QString>("resultFile");

	QTest::newRow("simple joins")
		<< true << 5.0 << 9 << 0.0 << "data/PolygonTest1-sample.png"
		<< "data/PolygonTest1-simple-joins-result.dat";
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

	QVERIFY(sampleImage.load(QFINDTESTDATA(imageFile)));

	polyTracer.setSimpleOnly(simpleOnly);
	polyTracer.setMaxDistance(maxDistance);
	polyTracer.setSpeckleSize(speckleSize);
	polyTracer.setDistDirRatio(distDirRatio);

	cove::Polygons::PolygonList polys;
	COVE_BENCHMARK
	{
		polys = polyTracer.createPolygonsFromImage(sampleImage);
	}

	//    saveResults(polys, QFINDTESTDATA(resultFile));
	compareResults(polys, QFINDTESTDATA(resultFile));
}

void PolygonTest::saveResults(const cove::Polygons::PolygonList& polys,
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
			out << double(p.x) << double(p.y);
	}
}

void PolygonTest::compareResults(const cove::Polygons::PolygonList& polys,
								 const QString& filename) const
{
	QFile file(filename);
	QVERIFY(file.open(QIODevice::ReadOnly));
	QDataStream in(&file);

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
			if (!qIsNull(p.x) && !qIsNull(x))
				QCOMPARE(p.x, x);
			if (!qIsNull(p.y) && !qIsNull(y))
				QCOMPARE(p.y, y);
		}
	}
}

QTEST_GUILESS_MAIN(PolygonTest)
