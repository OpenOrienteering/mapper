/*
 *    Copyright 2012-2024 Kai Pastor
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

#ifndef OPENORIENTEERING_GEOREFERENCING_TEST_H
#define OPENORIENTEERING_GEOREFERENCING_TEST_H

#include <QObject>
#include <QString>


/**
 * @test Tests the Georeferencing class.
 */
class GeoreferencingTest: public QObject
{
Q_OBJECT
	
private slots:
	/**
	 * Initializes shared data.
	 */
	void initTestCase();
	
	/**
	 * Tests the behaviour for unset projected CRS.
	 */
	void testEmptyProjectedCRS();
	
	/**
	 * Tests supported state changes.
	 */
	void testStateChanges();
	
	/**
	 * Tests the effect of a grid scale factor.
	 */
	void testGridScaleFactor();
	
	void testGridScaleFactor_data();
	
	/**
	 * Tests whether Georeferencing supports particular projected CRS.
	 */
	void testCRS();
	
	void testCRS_data();
	
	/**
	 * Tests CRS templates.
	 */
	void testCRSTemplates();
	
	/**
	 * Tests whether Georeferencing can convert geographic coordinates to
	 * projected coordinates and vice versa for a collection of known points.
	 */
	void testProjection();
	
	void testProjection_data();
	
#ifndef ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
	/**
	 * Tests whether the `proj_context_set_file_finder()` function is working.
	 * 
	 * Mapper relies on this function to extract data files on Android.
	 * This test covers this function in native builds.
	 */
	void testProjContextSetFileFinder();
#endif
	
	/**
	 * Tests the determination of the UTM zone from the given latitude and longitude.
	 */
	void testUTMZoneCalculation();
	
	void testUTMZoneCalculation_data();
};

#endif
