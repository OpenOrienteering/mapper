/*
 *    Copyright 2013-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_MAP_T_H
#define OPENORIENTEERING_MAP_T_H

#include <QObject>


/**
 * @test Tests the Map class. Still very incomplete
 *
 * @todo Extent this test.
 */
class MapTest : public QObject
{
Q_OBJECT
private slots:
	void initTestCase();
	
	/** Test icon scaling */
	void iconTest();
	 
	/** Tests basic print configuration properties. */
	void printerConfigTest();
	
	/** Tests if special colors are correctly handled. */
	void specialColorsTest();
	
	/** Tests various modes of Map::importMap(). */
	void importTest_data();
	void importTest();
	
	/** Tests hasAlpha() functions. */
	void hasAlpha();
	
	/** Basic tests for symbol set replacements. */
	void crtFileTest();
	
	/** Tests symbol set replacements with example files. */
	void matchQuerySymbolNumberTest_data();
	void matchQuerySymbolNumberTest();
	
};

#endif
