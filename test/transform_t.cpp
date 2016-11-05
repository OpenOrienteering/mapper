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

#include "../src/transformation.h"
#include "../src/template.h"


TransformTest::TransformTest(QObject* parent)
: QObject(parent)
{
	// nothing
}


void TransformTest::testTransformIdentity()
{
	QTransform qt;
	QCOMPARE(int(qt.type()), int(QTransform::TxNone));
	
	TemplateTransform t;
	qTransformToTemplateTransform(qt, &t);
	QCOMPARE(t.template_x, 0);
	QCOMPARE(t.template_y, 0);
	QCOMPARE(t.template_scale_x, 1.0);
	QCOMPARE(t.template_scale_y, 1.0);
	QCOMPARE(t.template_rotation, 0.0);
}

void TransformTest::testTransformTranslate()
{
	MapCoord offset { 100.0, 100.0 };
	QTransform qt;
	qt.translate(offset.x(), offset.y());
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
	QCOMPARE(t.template_rotation, rotation);
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


QTEST_APPLESS_MAIN(TransformTest)
