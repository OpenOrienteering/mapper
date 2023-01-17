/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2023 Kai Pastor
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


#ifndef OPENORIENTEERING_COLOR_LIST_WIDGET_H
#define OPENORIENTEERING_COLOR_LIST_WIDGET_H

#include <QObject>
#include <QString>
#include <QWidget>

class QAction;
class QIcon;
class QShowEvent;
class QTableWidget;
class QToolButton;

namespace OpenOrienteering {

class MainWindow;
class Map;
class MapColor;
class MapEditorController;


/**
 * A widget showing the list of map colors and some edit buttons at the bottom.
 * Enables to define new colors and edit or delete existing colors.
 * This widget is used inside a dock widget.
 */
class ColorListWidget : public QWidget
{
Q_OBJECT
public:
	/** Creates a new ColorWidget for a given map and MainWindow. */
	ColorListWidget(Map* map, MainWindow* window, QWidget* parent = nullptr, MapEditorController* map_edditor_controller = nullptr);
	
	/** Destroys the ColorWidget. */
	~ColorListWidget() override;
	
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
	
	void colorAdded(int index, const OpenOrienteering::MapColor* color);
	void colorChanged(int index, const OpenOrienteering::MapColor* color);
	void colorDeleted(int index, const OpenOrienteering::MapColor* color);
	
protected:
	void showEvent(QShowEvent* event) override;
	
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
	MapEditorController* const map_editor_controller;
	bool react_to_changes;
};


}  // namespace OpenOrienteering

#endif
