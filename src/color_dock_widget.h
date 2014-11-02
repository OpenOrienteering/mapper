/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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

#include <QtWidgets>

class ColorDropDown;
class MainWindow;
class Map;
class MapColor;
class PaletteColor;

/** 
 * A widget showing the list of map colors and some edit buttons at the bottom.
 * Enables to define new colors and edit or delete existing colors.
 * This widget is used inside a dock widget.
 */
class ColorWidget : public QWidget
{
Q_OBJECT
public:
  /** Creates a new ColorWidget for a given map and MainWindow. */
	ColorWidget(Map* map, MainWindow* window, QWidget* parent = NULL);
  
  /** Destroys the ColorWidget. */
	virtual ~ColorWidget();
	
protected slots:
	void newColor();
	void deleteColor();
	void duplicateColor();
	void moveColorUp();
	void moveColorDown();
	void editCurrentColor();
	void showHelp() const;
	
	void cellChange(int row, int column);
	void currentCellChange(int current_row, int current_column, int previous_row, int previous_column);
	
	void colorAdded(int index, const MapColor* color);
	void colorChanged(int index, const MapColor* color);
	void colorDeleted(int index, const MapColor* color);
	
protected:
	QToolButton* newToolButton(const QIcon& icon, const QString& text);
	
private:
	void addRow(int row);
	void updateRow(int row);
	
	// Color list
	QTableWidget* color_table;
	
	QAction* duplicate_action;
	
	// Buttons
	QToolButton* delete_button;
	QToolButton* move_up_button;
	QToolButton* move_down_button;
	QToolButton* edit_button;
	
	Map* const map;
	MainWindow* const window;
	bool react_to_changes;
};

#endif
