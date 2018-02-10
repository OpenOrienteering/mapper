/*
 *    Copyright 2012, 2013 Kai Pastor
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

#ifndef OPENORIENTEERING_MAP_COLOR_T_H
#define OPENORIENTEERING_MAP_COLOR_T_H

#include <QObject>


/**
 * @test Tests MapColor properties.
 */
class MapColorTest : public QObject
{
Q_OBJECT
public:
	/** Constructor */
	explicit MapColorTest(QObject* parent = nullptr);
	
private slots:
	/** Tests the constructors. */
	void constructorTest();
	
	/** Tests the equals method for some simple changes. */
	void equalsTest();
	
	/** Tests spot colors */
	void spotColorTest();
	
	/** Tests other feature */
	void miscTest();
	
};

#endif
