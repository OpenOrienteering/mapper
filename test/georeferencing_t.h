/*
 *    Copyright 2012 Kai Pastor
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

#ifndef _OPENORIENTEERING_TEST_GEOREFERENCING_H
#define _OPENORIENTEERING_TEST_GEOREFERENCING_H

#include <QObject>

#include "../src/georeferencing.h"


class GeoreferencingTest: public QObject
{
Q_OBJECT
	
private slots:
	void testEmptyProjectedCRS();
	
private:
	Georeferencing georef;
};

#endif
