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


#ifndef _OPENORIENTEERING_COLOR_DOCK_WIDGET_H_
#define _OPENORIENTEERING_COLOR_DOCK_WIDGET_H_

#include "map_editor.h"

class QBoxLayout;
class QPushButton;
class QTableWidget;
class QWidget;

class Map;

class ColorWidget : public EditorDockWidgetChild
{
Q_OBJECT
public:
	ColorWidget(Map* map, MainWindow* window, QWidget* parent = NULL);
    virtual ~ColorWidget();
	
    virtual QSize sizeHint() const;
	
protected:
    virtual void resizeEvent(QResizeEvent* event);
	
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
	
private:
	void addRow(int row);
	void updateRow(int row);
	
	bool wide_layout;
	QBoxLayout* layout;
	
	// Color list
	QTableWidget* color_table;
	
	// Buttons
	QWidget* buttons_group;
	QPushButton* delete_button;
	QPushButton* duplicate_button;
	QPushButton* move_up_button;
	QPushButton* move_down_button;
	
	Map* map;
	MainWindow* window;
	bool react_to_changes;
};

#endif
