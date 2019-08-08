/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#ifndef OPENORIENTEERING_DUPLICATE_EQUALS_T_H
#define OPENORIENTEERING_DUPLICATE_EQUALS_T_H

#include <QObject>
// IWYU pragma: no_include <QString>


/**
 * @test Test that duplicates of symbols and objects are equal to their originals.
 */
class DuplicateEqualsTest : public QObject
{
Q_OBJECT
private slots:
	void initTestCase();
	
	void symbols();
	void symbols_data();
	
	void objects();
	void objects_data();
};

#endif
