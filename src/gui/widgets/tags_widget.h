/*
 *    Copyright 2013 Kai Pastor
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


#ifndef OPENORIENTEERING_TAGS_WIDGET_H
#define OPENORIENTEERING_TAGS_WIDGET_H

#include <QObject>
#include <QString>
#include <QWidget>

class QIcon;
class QTableWidget;
class QToolButton;

namespace OpenOrienteering {

class Map;
class MapEditorController;
class MapView;
class Object;


/**
 * This widget allows for display and editing of tags, i.e. key-value pairs.
 */
class TagsWidget : public QWidget
{
Q_OBJECT
public:
	/** Constructs a new tags widget for the given map. */
	explicit TagsWidget(Map* map, MapView* main_view, MapEditorController* controller, QWidget* parent = nullptr);
	
	/** Destroys the widget. */
	~TagsWidget() override;
	
protected slots:
	/**
	 * Updates the widget from the current object's tags.
	 * 
	 * Does nothing if react_to_changes is set to false.
	 */
	void objectTagsChanged();
	
	/**
	 * Updates the current object's tags from the widget.
	 * 
	 * Does nothing if react_to_changes is set to false.
	 */
	void cellChange(int row, int column);
	
	/**
	 * Opens the help page for this widget.
	 */
	void showHelp();
	
protected:
	/** 
	 * Sets up the last row: blank cells, right one not editable.
	 */
	void setupLastRow();
	
	/**
	 * Creates and registers an undo step for the given object.
	 * 
	 * @param object must not be null.
	 */
	void createUndoStep(Object* object);
	
private:
	Map* map;
	MapView* main_view;
	MapEditorController* controller;
	bool react_to_changes;
	
	QTableWidget* tags_table;
};


}  // namespace OpenOrienteering

#endif // TAGS_WIDGET_H
