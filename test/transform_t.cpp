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

#include <QtTest/QtTest>
#include <QTransform>

#include "util/transformation.h"
#include "../src/templates/template.h"


Q_STATIC_ASSERT(std::is_standard_layout<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_default_constructible<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_copy_constructible<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_move_constructible<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_trivially_copyable<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_copy_assignable<TemplateTransform>::value);
Q_STATIC_ASSERT(std::is_nothrow_move_assignable<TemplateTransform>::value);


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
	qTransformToTemplateTransform(qt, &t);
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
	
	TemplateTransform t;
	qTransformToTemplateTransform(qt, &t);
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
	
	TemplateTransform t;
	qTransformToTemplateTransform(qt, &t);
	QCOMPARE(t.template_x, 0);
	QCOMPARE(t.template_y, 0);
	QCOMPARE(t.template_scale_x, scale_x);
	QCOMPARE(t.template_scale_y, scale_y);
	QCOMPARE(t.template_rotation, 0.0);
}

void TransformTest::testTransformRotate()
{
	qreal rotation = 0.1;
	QTransform qt;
	qt.rotateRadians(rotation, Qt::ZAxis);
	QCOMPARE(int(qt.type()), int(QTransform::TxRotate));
	
	TemplateTransform t;
	qTransformToTemplateTransform(qt, &t);
	QCOMPARE(t.template_x, 0);
	QCOMPARE(t.template_y, 0);
	QCOMPARE(t.template_scale_x, 1.0);
	QCOMPARE(t.template_scale_y, 1.0);
	QCOMPARE(t.template_rotation, -rotation);
}

void TransformTest::testEstimateNonIsometric()
{
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
	
	TemplateTransform t;
	qTransformToTemplateTransform(qt, &t);
	QCOMPARE(t.template_x, MapCoord(32,64).nativeX());
	QCOMPARE(t.template_y, MapCoord(32,64).nativeY());
	QCOMPARE(t.template_scale_x, 0.25);
	QCOMPARE(t.template_scale_y, 0.25);
	QVERIFY(t.template_rotation < 0.000001);
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
		
		TemplateTransform check;
		qTransformToTemplateTransform(q0, &check);
		QCOMPARE(check, t0);
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
		
		TemplateTransform check;
		qTransformToTemplateTransform(q1, &check);
		QCOMPARE(check, t1);
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
		
		QCOMPARE(QPointF(passpoints[0].calculated_coords), QPointF(passpoints[0].dest_coords));
		QVERIFY(passpoints[0].error < min_distance);
		QCOMPARE(QPointF(passpoints[1].calculated_coords), QPointF(passpoints[1].dest_coords));
		QVERIFY(passpoints[1].error < min_distance);
		
		QTransform q2;
		QVERIFY(passpoints.estimateSimilarityTransformation(&q2));
		QVERIFY(q2.isAffine());
		QVERIFY(q2.isRotating());
		QCOMPARE(q2.dx(), qreal(0));
		QCOMPARE(q2.dy(), qreal(0));
		
		QCOMPARE(QPointF(passpoints[0].calculated_coords), QPointF(passpoints[0].dest_coords));
		QVERIFY(passpoints[0].error < min_distance);
		QCOMPARE(QPointF(passpoints[1].calculated_coords), QPointF(passpoints[1].dest_coords));
		QVERIFY(passpoints[1].error < min_distance);
		
		TemplateTransform check;
		qTransformToTemplateTransform(q2, &check);
		QCOMPARE(check, t2);
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
		
		TemplateTransform check;
		qTransformToTemplateTransform(q3, &check);
		QCOMPARE(check, t3);
	}
}


QTEST_APPLESS_MAIN(TransformTest)
