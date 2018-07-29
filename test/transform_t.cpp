/*
 *    Copyright 2016 Kai Pastor
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


#include "transform_t.h"

#include <QtTest>
#include <QTransform>

#include "templates/template.h"
#include "util/transformation.h"

using namespace OpenOrienteering;


#if QT_VERSION == 0x050B01
// Qt 5.11.1 has a regression in QPointF::operator==.
// References:
// https://bugreports.qt.io/browse/QTBUG-69368
// https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=903237
// https://github.com/OpenOrienteering/mapper/issues/1116
#  define QTBUG_69368_QUIRK
#endif


TransformTest::TransformTest(QObject* parent)
: QObject(parent)
{
	// nothing
}

void TransformTest::testTransformBasics()
{
	// Default TemplateTransform: identity
	TemplateTransform t;
	QCOMPARE(t.template_x, 0);
	QCOMPARE(t.template_y, 0);
	QCOMPARE(t.template_scale_x, 1.0);
	QCOMPARE(t.template_scale_y, 1.0);
	QCOMPARE(t.template_rotation, 0.0);
	
	// List initialization
	auto t2 = TemplateTransform { 12, -3, 0.1, 2.0, 3.0 };
	QCOMPARE(t2.template_x, 12);
	QCOMPARE(t2.template_y, -3);
	QCOMPARE(t2.template_scale_x, 2.0);
	QCOMPARE(t2.template_scale_y, 3.0);
	QCOMPARE(t2.template_rotation, 0.1);
	
	// Assignment
	t = t2;
	QCOMPARE(t.template_x, 12);
	QCOMPARE(t.template_y, -3);
	QCOMPARE(t.template_scale_x, 2.0);
	QCOMPARE(t.template_scale_y, 3.0);
	QCOMPARE(t.template_rotation, 0.1);
	
	// Comparison
	QCOMPARE(t, t2);
}


void TransformTest::testTransformIdentity()
{
	// Default QTransform: identity
	QTransform qt;
	QVERIFY(qt.isIdentity());
	QCOMPARE(int(qt.type()), int(QTransform::TxNone));
	
	// Default TemplateTransform: identity
	TemplateTransform t;
	
	// TemplateTransform list initialization, and assignment
	t = { 12, -3, 0.1, 2.0, 3.0 };
	QCOMPARE(t.template_x, 12);
	QCOMPARE(t.template_y, -3);
	QCOMPARE(t.template_scale_x, 2.0);
	QCOMPARE(t.template_scale_y, 3.0);
	QCOMPARE(t.template_rotation, 0.1);
	
	// Now transfer the identity QTransform to the TemplateTransform
	t = TemplateTransform::fromQTransform(qt);
	QCOMPARE(t.template_x, 0);
	QCOMPARE(t.template_y, 0);
	QCOMPARE(t.template_scale_x, 1.0);
	QCOMPARE(t.template_scale_y, 1.0);
	QCOMPARE(t.template_rotation, 0.0);
	
	// Put something different in the QTransform
	qt.translate(4, 8);
	qt.rotate(1);
	qt.scale(2.0, 1.5);
	QVERIFY(qt.isAffine());
	QVERIFY(!qt.isIdentity());
	QVERIFY(qt.isTranslating());
	QVERIFY(qt.isRotating());
	QVERIFY(qt.isScaling());
}

void TransformTest::testTransformTranslate()
{
	MapCoord offset { 100.0, 100.0 };
	QTransform qt;
	qt.translate(offset.x(), offset.y());
	QVERIFY(qt.isTranslating());
	QCOMPARE(int(qt.type()), int(QTransform::TxTranslate));
	
	auto t = TemplateTransform::fromQTransform(qt);
	QCOMPARE(t.template_x, offset.nativeX());
	QCOMPARE(t.template_y, offset.nativeY());
	QCOMPARE(t.template_scale_x, 1.0);
	QCOMPARE(t.template_scale_y, 1.0);
	QCOMPARE(t.template_rotation, 0.0);
}

void TransformTest::testTransformProject()
{
	qreal scale_x = 4.5;
	qreal scale_y = 2.5;
	QTransform qt;
	qt.scale(scale_x, scale_y);
	QVERIFY(qt.isScaling());
	QCOMPARE(int(qt.type()), int(QTransform::TxScale));
	
	auto t = TemplateTransform::fromQTransform(qt);

	QCOMPARE(t.template_x, 0);
	QCOMPARE(t.template_y, 0);
	QCOMPARE(t.template_scale_x, scale_x);
	QCOMPARE(t.template_scale_y, scale_y);
	QCOMPARE(t.template_rotation, 0.0);
}

void TransformTest::testTransformRotate_data()
{
	QTest::addColumn<int>("type");
	QTest::addColumn<double>("rotation");
	
	QTest::newRow("No rotation") << int(QTransform::TxNone)   <<   0.0;
	QTest::newRow("90 deg")      << int(QTransform::TxRotate) <<  90.0; // cos(90deg) := 0
	QTest::newRow("180 deg")     << int(QTransform::TxScale)  << 180.0; // sin(180deg) := 0
	QTest::newRow("200 deg")     << int(QTransform::TxRotate) << 200.0; // quadrant III
	QTest::newRow("270 deg")     << int(QTransform::TxRotate) << 270.0; // cos(270deg) := 0
}

void TransformTest::testTransformRotate()
{
	QFETCH(int, type);
	QFETCH(double, rotation);
	
	QTransform qt;
	qt.rotate(qreal(rotation), Qt::ZAxis);
	QCOMPARE(int(qt.type()), type);
	
	auto t = TemplateTransform::fromQTransform(qt);
	QCOMPARE(t.template_x, 0);
	QCOMPARE(t.template_y, 0);
	QCOMPARE(t.template_scale_x, 1.0);
	QCOMPARE(t.template_scale_y, 1.0);
	if (rotation <= 180.0)
		QCOMPARE(t.template_rotation, -qDegreesToRadians(qreal(rotation)));
	else
		QCOMPARE(t.template_rotation, -qDegreesToRadians(qreal(rotation - 360.0)));
}

void TransformTest::testTransformCombined_data()
{
	QTest::addColumn<int>("type");
	QTest::addColumn<double>("rotation");
	
	QTest::newRow("No rotation") << int(QTransform::TxScale)  <<   0.0;
	QTest::newRow("90 deg")      << int(QTransform::TxRotate) <<  90.0; // cos(90deg) := 0
	QTest::newRow("180 deg")     << int(QTransform::TxScale)  << 180.0; // sin(180deg) := 0
	QTest::newRow("200 deg")     << int(QTransform::TxRotate) << 200.0; // quadrant III
	QTest::newRow("270 deg")     << int(QTransform::TxRotate) << 270.0; // cos(270deg) := 0
}

void TransformTest::testTransformCombined()
{
	QFETCH(int, type);
	QFETCH(double, rotation);
	
	qint32 dx = -3000;
	qint32 dy = 16000;
	qreal scale_x = 4.5;
	qreal scale_y = 2.5;
	
	QTransform qt;
	// Template::applyTemplateTransform order: translate, rotate, scale
	qt.translate(dx / 1000.0, dy / 1000.0);
	qt.rotate(qreal(rotation));
	qt.scale(scale_x, scale_y);
	QVERIFY(qt.isScaling());
	QCOMPARE(int(qt.type()), type);
	
	auto t = TemplateTransform::fromQTransform(qt);
	QCOMPARE(t.template_x, dx);
	QCOMPARE(t.template_y, dy);
	QCOMPARE(t.template_scale_x, scale_x);
	QCOMPARE(t.template_scale_y, scale_y);
	if (rotation <= 180.0)
		QCOMPARE(t.template_rotation, -qDegreesToRadians(qreal(rotation)));
	else
		QCOMPARE(t.template_rotation, -qDegreesToRadians(qreal(rotation - 360.0)));
}

void TransformTest::testTransformRoundTrip_data()
{
	QTest::addColumn<int>("dx");
	QTest::addColumn<int>("dy");
	QTest::addColumn<double>("rotation");
	QTest::addColumn<double>("scale_x");
	QTest::addColumn<double>("scale_y");
	
	// For simplicity, we use rotation from -180 to 180 degree,
	QTest::newRow("No rotation") << 120 << -20 <<    0.0 << 1.2 << 0.7;
	QTest::newRow("90 deg")      << 320 << -60 <<   90.0 << 1.3 << 1.7;
	QTest::newRow("180 deg")     << -20 << 220 <<  180.0 << 1.4 << 0.5;
	QTest::newRow("-160 deg")    << 100 << -10 << -160.0 << 1.5 << 2.7;
	QTest::newRow("-90 deg")     <<   0 << -20 <<  -90.0 << 1.6 << 3.7;
}

void TransformTest::testTransformRoundTrip()
{
	QFETCH(int, dx);
	QFETCH(int, dy);
	QFETCH(double, rotation);
	QFETCH(double, scale_x);
	QFETCH(double, scale_y);

	TemplateTransform t = { dx, dy, qDegreesToRadians(rotation), scale_x, scale_y };
	
	// Template::applyTemplateTransform order: translate, rotate, scale
	QTransform qt;
	qt.translate(t.template_x / 1000.0, t.template_y / 1000.0);
	qt.rotate(-t.template_rotation * (180 / M_PI));
	qt.scale(t.template_scale_x, t.template_scale_y);
	
	auto t2 = TemplateTransform::fromQTransform(qt);
	QCOMPARE(t2, t);
}

void TransformTest::testEstimateNonIsometric()
{
	// Scaling and translation
	PassPointList passpoints;
	passpoints.resize(3);
	passpoints[0].src_coords  = MapCoordF { 128.0, 0.0 };
	passpoints[0].dest_coords = MapCoordF { 64.0, 64.0 };
	passpoints[1].src_coords  = MapCoordF { 256.0, 0.0 };
	passpoints[1].dest_coords = MapCoordF { 96.0, 64.0 };
	passpoints[2].src_coords  = MapCoordF { 128.0, 128.0 };
	passpoints[2].dest_coords = MapCoordF { 64.0, 96.0 };
	
	QTransform qt;
	QVERIFY(passpoints.estimateNonIsometricSimilarityTransform(&qt));
	QVERIFY(qt.isTranslating());
	QVERIFY(qt.isScaling());
	QCOMPARE(int(qt.type()), int(QTransform::TxScale));
	
	QCOMPARE(qt.map(passpoints[0].src_coords), QPointF{passpoints[0].dest_coords});
	QCOMPARE(qt.map(passpoints[1].src_coords), QPointF{passpoints[1].dest_coords});
	QCOMPARE(qt.map(passpoints[2].src_coords), QPointF{passpoints[2].dest_coords});
	
	auto t = TemplateTransform::fromQTransform(qt);
	QCOMPARE(t.template_x, MapCoord(32,64).nativeX());
	QCOMPARE(t.template_y, MapCoord(32,64).nativeY());
	QCOMPARE(t.template_scale_x, 0.25);
	QCOMPARE(t.template_scale_y, 0.25);
	QVERIFY(qAbs(t.template_rotation) < 0.000001);
	
	// Rotation
	passpoints[0].src_coords  = MapCoordF { 0.0, 0.0 };
	passpoints[0].dest_coords = MapCoordF { 0.0, 0.0 };
	passpoints[1].src_coords  = MapCoordF { 5.0, 0.0 };
	passpoints[1].dest_coords = MapCoordF { 4.0, -3.0 };
	passpoints[2].src_coords  = MapCoordF { 0.0, 5.0 };
	passpoints[2].dest_coords = MapCoordF { 3.0, 4.0 };
	
	QVERIFY(passpoints.estimateNonIsometricSimilarityTransform(&qt));
	QVERIFY(qt.isRotating());
	QCOMPARE(int(qt.type()), int(QTransform::TxRotate));
	
	QCOMPARE(qt.map(passpoints[0].src_coords), QPointF{passpoints[0].dest_coords});
	QCOMPARE(qt.map(passpoints[1].src_coords), QPointF{passpoints[1].dest_coords});
	QCOMPARE(qt.map(passpoints[2].src_coords), QPointF{passpoints[2].dest_coords});
	
	t = TemplateTransform::fromQTransform(qt);
	QCOMPARE(t.template_x, 0);
	QCOMPARE(t.template_y, 0);
	QCOMPARE(t.template_scale_x, 1.0);
	QCOMPARE(t.template_scale_y, 1.0);
	QCOMPARE(t.template_rotation, qAcos(passpoints[1].dest_coords.x() / passpoints[1].src_coords.x()));
}

void TransformTest::testEstimateSimilarityTransformation()
{
	const auto min_distance = MapCoordF{0,0}.distanceTo({0.001,0});
	
	PassPointList passpoints;
	passpoints.reserve(3);
	
	{
		passpoints.resize(0);
		
		TemplateTransform t0;
		QVERIFY(passpoints.estimateSimilarityTransformation(&t0));
		
		QTransform q0;
		QVERIFY(passpoints.estimateSimilarityTransformation(&q0));
		QVERIFY(q0.isIdentity());
		
		QCOMPARE(TemplateTransform::fromQTransform(q0), t0);
	}
	
	{
		passpoints.resize(1);
		passpoints[0].src_coords  = MapCoordF { 32.0, 0.0 };
		passpoints[0].dest_coords = MapCoordF { 64.0, 64.0 };
		
		TemplateTransform t1;
		QVERIFY(passpoints.estimateSimilarityTransformation(&t1));
		QCOMPARE(t1.template_x, MapCoord(32,64).nativeX());
		QCOMPARE(t1.template_y, MapCoord(32,64).nativeY());
		QCOMPARE(t1.template_scale_x, 1.0);
		QCOMPARE(t1.template_scale_y, 1.0);
		QVERIFY(t1.template_rotation < 0.000001);
		
		QCOMPARE(QPointF(passpoints[0].calculated_coords), QPointF(passpoints[0].dest_coords));
		QVERIFY(passpoints[0].error < min_distance);
		
		QTransform q1;
		QVERIFY(passpoints.estimateSimilarityTransformation(&q1));
		QVERIFY(q1.isAffine());
		QVERIFY(q1.isTranslating());
		QVERIFY(!q1.isScaling());
		QVERIFY(!q1.isRotating());
		QCOMPARE(q1.dx(), MapCoord(32,64).x());
		QCOMPARE(q1.dy(), MapCoord(32,64).y());
		
		QCOMPARE(QPointF(passpoints[0].calculated_coords), QPointF(passpoints[0].dest_coords));
		QVERIFY(passpoints[0].error < min_distance);
		
		QCOMPARE(TemplateTransform::fromQTransform(q1), t1);
	}
	
	{
		passpoints.resize(2);
		passpoints[0].src_coords  = MapCoordF { 32.0, 0.0 };
		passpoints[0].dest_coords = MapCoordF { 0.0, -32.0 };
		passpoints[1].src_coords  = MapCoordF { 0.0, -32.0 };
		passpoints[1].dest_coords = MapCoordF { -32.0, 0.0 };
		
		TemplateTransform t2;
		QVERIFY(passpoints.estimateSimilarityTransformation(&t2));
		QCOMPARE(t2.template_x, 0);
		QCOMPARE(t2.template_y, 0);
		QCOMPARE(t2.template_rotation, qDegreesToRadians(90.0));
		QCOMPARE(t2.template_scale_x, 1.0);
		QCOMPARE(t2.template_scale_y, 1.0);
		
#ifndef QTBUG_69368_QUIRK
		QCOMPARE(QPointF(passpoints[0].calculated_coords), QPointF(passpoints[0].dest_coords));
		QVERIFY(passpoints[0].error < min_distance);
		QCOMPARE(QPointF(passpoints[1].calculated_coords), QPointF(passpoints[1].dest_coords));
		QVERIFY(passpoints[1].error < min_distance);
#else
		QWARN("Parts of test skipped to avoid hitting QTBUG-69368");
#endif // QT_VERSION != 0x050B01
		
		QTransform q2;
		QVERIFY(passpoints.estimateSimilarityTransformation(&q2));
		QVERIFY(q2.isAffine());
		QVERIFY(q2.isRotating());
		QCOMPARE(q2.dx(), qreal(0));
		QCOMPARE(q2.dy(), qreal(0));
		
#ifndef QTBUG_69368_QUIRK
		QCOMPARE(QPointF(passpoints[0].calculated_coords), QPointF(passpoints[0].dest_coords));
		QVERIFY(passpoints[0].error < min_distance);
		QCOMPARE(QPointF(passpoints[1].calculated_coords), QPointF(passpoints[1].dest_coords));
		QVERIFY(passpoints[1].error < min_distance);
#endif // QTBUG_69368_QUIRK
		
		QCOMPARE(TemplateTransform::fromQTransform(q2), t2);
	}
	
	{
		passpoints.resize(3);
		passpoints[0].src_coords  = MapCoordF { 128.0, 0.0 };
		passpoints[0].dest_coords = MapCoordF { 64.0, 64.0 };
		passpoints[1].src_coords  = MapCoordF { 256.0, 0.0 };
		passpoints[1].dest_coords = MapCoordF { 96.0, 64.0 };
		passpoints[2].src_coords  = MapCoordF { 128.0, 128.0 };
		passpoints[2].dest_coords = MapCoordF { 64.0, 96.0 };
		
		TemplateTransform t3;
		QVERIFY(passpoints.estimateSimilarityTransformation(&t3));
		QCOMPARE(t3.template_x, MapCoord(32,64).nativeX());
		QCOMPARE(t3.template_y, MapCoord(32,64).nativeY());
		QCOMPARE(t3.template_scale_x, 0.25);
		QCOMPARE(t3.template_scale_y, 0.25);
		QVERIFY(t3.template_rotation < 0.000001);
		
		QCOMPARE(QPointF(passpoints[0].calculated_coords), QPointF(passpoints[0].dest_coords));
		QVERIFY(passpoints[0].error < min_distance);
		QCOMPARE(QPointF(passpoints[1].calculated_coords), QPointF(passpoints[1].dest_coords));
		QVERIFY(passpoints[1].error < min_distance);
		QCOMPARE(QPointF(passpoints[2].calculated_coords), QPointF(passpoints[2].dest_coords));
		QVERIFY(passpoints[2].error < min_distance);
		
		QTransform q3;
		QVERIFY(passpoints.estimateSimilarityTransformation(&q3));
		QVERIFY(q3.isAffine());
		QVERIFY(q3.isScaling());
		QCOMPARE(q3.dx(), MapCoord(32,64).x());
		QCOMPARE(q3.dy(), MapCoord(32,64).y());
		
		QCOMPARE(QPointF(passpoints[0].calculated_coords), QPointF(passpoints[0].dest_coords));
		QVERIFY(passpoints[0].error < min_distance);
		QCOMPARE(QPointF(passpoints[1].calculated_coords), QPointF(passpoints[1].dest_coords));
		QVERIFY(passpoints[1].error < min_distance);
		QCOMPARE(QPointF(passpoints[2].calculated_coords), QPointF(passpoints[2].dest_coords));
		QVERIFY(passpoints[2].error < min_distance);
		
		QCOMPARE(TemplateTransform::fromQTransform(q3), t3);
	}
}


QTEST_APPLESS_MAIN(TransformTest)
