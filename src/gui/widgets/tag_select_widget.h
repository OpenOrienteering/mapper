/*
 *    Copyright 2016 Mitchell Krome
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


#ifndef OPENORIENTEERING_TAG_SELECT_WIDGET_H
#define OPENORIENTEERING_TAG_SELECT_WIDGET_H

#include <memory>

#include <QWidget>

class QBoxLayout;
class QLabel;
class QTableWidget;
class QToolButton;

class Map;
class MapEditorController;
class MapView;
class Object;
class ObjectQuery;

/**
 * This widget allows the user to make selections based on an objects tags.
 */
class TagSelectWidget : public QWidget
{
Q_OBJECT
public:
	TagSelectWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent = nullptr);
	~TagSelectWidget() override;
	
	void resetSelectionInfo();
	
protected:
	/**
	 * Returns a new QToolButton with a unified appearance.
	 */
	QToolButton* newToolButton(const QIcon& icon, const QString& text);
	
	void showHelp();
	void addRow();
	void deleteRow();
	void moveRow(bool up);
	void makeSelection();

private:
	void addRowItems(int row);
	void cellChanged(int row, int column);

	/**
	 * Builds a query based on the current state of the query table.
	 * 
	 * On error the unique_ptr contains nullptr.
	 */
	std::unique_ptr<ObjectQuery> makeQuery() const;
	
	Map* map;
	MapView* main_view;
	MapEditorController* controller;
	
	QTableWidget* query_table;
	QBoxLayout* all_query_layout;
	
	// Buttons
	QWidget* list_buttons_group;
	QToolButton* add_button;
	QToolButton* delete_button;
	QToolButton* move_up_button;
	QToolButton* move_down_button;
	QToolButton* select_button;
	QLabel* selection_info;
};

#endif
