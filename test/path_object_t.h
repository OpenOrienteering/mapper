/*
 *    Copyright 2013 Thomas Schöps
 *    Copyright 2015, 2016, 2019 Kai Pastor
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


#ifndef OPENORIENTEERING_PATH_OBJECT_T_H
#define OPENORIENTEERING_PATH_OBJECT_T_H

#include <QObject>


/**
 * @test Tests PathObject, MapCoord(F) and VirtualPath.
 *
 * @todo Extent this test.
 */
class PathObjectTest : public QObject
{
Q_OBJECT
public:
	/** Constructor */
	explicit PathObjectTest(QObject* parent = nullptr);
	
private slots:
	void initTestCase();
	
	/** Tests MapCoordF. */
	void mapCoordTest();
	
	/** Tests VirtualPath. */
	void virtualPathTest();
	
	/** Tests PathObject constructors. */
	void constructorTest();
	
	/** Tests PathObject::copyFrom(). */
	void copyFromTest();
	void copyFromTest_data();
	
	/** Basic test for PathObject::changePathBounds(), focus on flags. */
	void changePathBoundsBasicTest();
	void changePathBoundsBasicTest_data();
	
	/** Tests PathObject::changePathBounds(). */
	void changePathBoundsTest();
	void changePathBoundsTest_data();
	
	/** Tests properties of lines after splitting. */
	void splitLineTest();
	void splitLineTest_data();
	
	/** Tests PathObject::removeFromLine(). */
	void removeFromLineTest();
	void removeFromLineTest_data();
	
	/** Tests finding intersections with calcAllIntersectionsWith(). */
	void calcIntersectionsTest();
	
	/** Tests PathCoord and SplitPathCoord for a non-trivial zero-length path. */
	void atypicalPathTest();
	
};

#endif
