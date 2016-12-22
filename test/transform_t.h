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

#ifndef OPENORIENTEERING_COORD_T_H
#define OPENORIENTEERING_COORD_T_H

#include <QtTest/QtTest>


/**
 * @test Tests coordinate transformation.
 */
class TransformTest : public QObject
{
Q_OBJECT
public:
	explicit TransformTest(QObject* parent = NULL);
	
private slots:
	/**
	 * Tests conversion of identity QTransform to TemplateTransform
	 */
	void testTransformIdentity();
	
	/**
	 * Tests conversion of translating QTransform to TemplateTransform
	 */
	void testTransformTranslate();
	
	/**
	 * Tests conversion of projecting QTransform to TemplateTransform
	 */
	void testTransformProject();
	
	/**
	 * Tests conversion of rotating QTransform to TemplateTransform
	 */
	void testTransformRotate();
	
	/**
	 * Test PassPointList::estimateNonIsometricSimilarityTransform.
	 */
	void testEstimateNonIsometric();
};

#endif
