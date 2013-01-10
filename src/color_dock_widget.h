/*
 *    Copyright 2012 Thomas Schöps, Kai Pastor
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


#ifndef _OPENORIENTEERING_COLOR_DOCK_WIDGET_H_
#define _OPENORIENTEERING_COLOR_DOCK_WIDGET_H_

#include <qglobal.h>
#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

class ColorDropDown;
class MainWindow;
class Map;
class MapColor;
class PaletteColor;

class ColorWidget : public QWidget
{
Q_OBJECT

public:
	ColorWidget(Map* map, MainWindow* window, QWidget* parent = NULL);
	virtual ~ColorWidget();
	
protected slots:
	void newColor();
	void deleteColor();
	void duplicateColor();
	void moveColorUp();
	void moveColorDown();
	void showHelp();
	
	void cellChange(int row, int column);
	void currentCellChange(int current_row, int current_column, int previous_row, int previous_column);
	void cellDoubleClick(int row, int column);
	
	void colorAdded(int index, MapColor* color);
	void colorDeleted(int index, MapColor* color);
	
protected:
	QAbstractButton* newToolButton(const QIcon& icon, const QString& text, QAbstractButton* prototype = NULL);
	
private:
	void addRow(int row);
	void updateRow(int row);
	
	// Color list
	QTableWidget* color_table;
	
	// Buttons
	QAbstractButton* delete_button;
	QAbstractButton* duplicate_button;
	QAbstractButton* move_up_button;
	QAbstractButton* move_down_button;
	
	Map* map;
	MainWindow* window;
	bool react_to_changes;
};

#endif
