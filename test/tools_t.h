/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017, 2020, 2025 Kai Pastor
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

#ifndef OPENORIENTEERING_TOOLS_T_H
#define OPENORIENTEERING_TOOLS_T_H

#include <QObject>


/**
 * @test Tests the editing tools.
 */
class ToolsTest : public QObject
{
Q_OBJECT
private slots:
	void initTestCase();
	
	void editTool();
	
	void paintOnTemplateFeature();
	
	void testFindObjects();
};

#endif
