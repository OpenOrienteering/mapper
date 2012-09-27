/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef _OPENORIENTEERING_TEST_H_
#define _OPENORIENTEERING_TEST_H_

#include <QtTest/QtTest>

class Map;
class MainWindow;
class MapWidget;
class MapEditorController;
class Format;
class LineSymbol;
class PathObject;

/// Creates a test map and provides pointers to specific map elements.
/// NOTE: delete the map manually in case its ownership is not transferred to a MapEditorController or similar!
struct TestMap
{
	Map* map;
	
	LineSymbol* line_symbol;
	PathObject* line_object;
	
	TestMap();
	~TestMap();
};

/// Creates a test map editor and provides related pointers.
struct TestMapEditor
{
	MainWindow* window;
	MapEditorController* editor;
	MapWidget* map_widget;
	
	TestMapEditor(Map* map);
	~TestMapEditor();
	
	void simulateClick(QPoint pos);
	void simulateClick(QPointF pos);
	void simulateDrag(QPoint start_pos, QPoint end_pos);
	void simulateDrag(QPointF start_pos, QPointF end_pos);
};

/// Ensures that maps contain the same information before and after saving them and loading them again.
class TestFileFormats : public QObject
{
Q_OBJECT
private slots:
	void saveAndLoad_data();
	void saveAndLoad();
	
private:
	Map* saveAndLoadMap(Map* input, const Format* format);
	bool compareMaps(Map* a, Map* b, QString& error);
};

/// Ensures that duplicates of symbols and objects are equal to their originals.
class TestDuplicateEqual : public QObject
{
Q_OBJECT
private slots:
	void symbols_data();
	void symbols();
	
	void objects_data();
	void objects();
};

/// Tests the editing tools
class TestTools : public QObject
{
Q_OBJECT
private slots:
	void editTool();
};


#endif
