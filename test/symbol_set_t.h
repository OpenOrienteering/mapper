/*
 *    Copyright 2014 Kai Pastor
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

#ifndef _OPENORIENTEERING_SYMBOL_SET_T_H
#define _OPENORIENTEERING_SYMBOL_SET_T_H

#include <QtTest/QtTest>

/**
 * @test Creates compressed and scaled symbol sets from original symbol sets.
 * 
 * This tool overwrites the .omap symbol set files in the symbol set directory.
 * This is not only a test uncovering changes in the standard file format but
 * also helps to maintain the released compact symbol sets from a minimum set
 * of verbose source symbol sets.
 */
class SymbolSetTool : public QObject
{
Q_OBJECT
	
private slots:
	void initTestCase();
	void symbolSetDirExists();
	void processSymbolSet_data();
	void processSymbolSet();
	
private:
	QDir symbol_set_dir;
};

#endif // _OPENORIENTEERING_SYMBOL_SET_T_H
