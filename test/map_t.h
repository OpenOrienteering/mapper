/*
 *    Copyright 2013 Kai Pastor
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


#ifndef _OPENORIENTEERING_MAP_T_H
#define _OPENORIENTEERING_MAP_T_H

#include <QtTest/QtTest>

#include "../src/object.h"


/**
 * Tests the Map class. Still very incomplete (TODO).
 */
class MapTest : public QObject
{
Q_OBJECT
public:
	/** Constructor. */
	explicit MapTest(QObject* parent = NULL);
	
private slots:
	void initTestCase();
	
	/** Tests if special colors are correctly handled. */
	void specialColorsTest();
	
};

#endif
