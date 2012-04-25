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


#ifndef _OPENORIENTEERING_SYMBOL_POINT_EDITOR_H_
#define _OPENORIENTEERING_SYMBOL_POINT_EDITOR_H_

#include <QWidget>

#include "map_editor.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QTableWidget;

class PointSymbolEditorActivity;
class PointObject;
class PointSymbol;
class ColorDropDown;
class Symbol;
class Object;
class Map;

class PointSymbolEditorWidget : public QWidget
{
Q_OBJECT
friend class PointSymbolEditorActivity;
public:
	PointSymbolEditorWidget(MapEditorController* controller, PointSymbol* symbol, float offset_y = 0, bool permanent_preview = false, QWidget* parent = 0);
	
	virtual ~PointSymbolEditorWidget();
	
	bool changeCurrentCoordinate(MapCoordF new_coord);	// returns true if successful
	bool addCoordinate(MapCoordF new_coord);			// returns true if successful
	
	void setEditorActive(bool active);
	
	virtual void setVisible(bool visible);
	
signals:
	void symbolEdited();
	
private slots:
	void updateElementList();
	
	void elementChanged(int row);
	
	void addPointClicked();
	void addLineClicked();
	void addAreaClicked();
	void deleteElementClicked();
	
	void pointInnerRadiusChanged(QString text);
	void pointInnerColorChanged();
	void pointOuterWidthChanged(QString text);
	void pointOuterColorChanged();
	
	void lineWidthChanged(QString text);
	void lineColorChanged();
	void lineCapChanged(int index);
	void lineJoinChanged(int index);
	void lineClosedClicked(bool checked);
	
	void areaColorChanged();
	
	void currentCoordChanged();
	void coordinateChanged(int row, int column);
	void addCoordClicked();
	void deleteCoordClicked();
	void centerCoordsClicked();
	
private:
	void updateCoordsTable();
	void addCoordsRow(int row);
	void updateCoordsRow(int row);
	void updateDeleteCoordButton();
	
	void insertElement(Object* object, Symbol* symbol);
	QString getLabelForSymbol(Symbol* symbol) const;
	
	Symbol* getCurrentElementSymbol();
	Object* getCurrentElementObject();
	
	PointSymbol* const symbol;
	PointObject* midpoint_object;
	const MapCoordF object_origin_coord;
	
	QListWidget* element_list;
	QPushButton* delete_element_button;
	
	QStackedWidget* element_properties_widget;
	
	QLineEdit* point_inner_radius_edit;
	ColorDropDown* point_inner_color_edit;
	QLineEdit* point_outer_width_edit;
	ColorDropDown* point_outer_color_edit;
	
	QLineEdit* line_width_edit;
	ColorDropDown* line_color_edit;
	QComboBox* line_cap_edit;
	QComboBox* line_join_edit;
	QCheckBox* line_closed_check;
	
	ColorDropDown* area_color_edit;
	
	QLabel* coords_label;
	QTableWidget* coords_table;
	QPushButton* add_coord_button;
	QPushButton* delete_coord_button;
	QPushButton* center_coords_button;
	
	bool react_to_changes;
	const float offset_y;
	PointSymbolEditorActivity* activity;
	Map* map;
	MapEditorController* controller;
	const bool permanent_preview;
};



/** PointSymbolEditorActivity allows to add or modify coordinates of point symbol elements
 *  by clicking in the map.
 */
class PointSymbolEditorTool : public MapEditorTool
{
Q_OBJECT
public:
	PointSymbolEditorTool(MapEditorController* editor, PointSymbolEditorWidget* symbol_editor);
	
	virtual void init();
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* map_widget);
	virtual QCursor* getCursor() { return cursor; }; // FIXME: const candidate
	
private:
	PointSymbolEditorWidget* const symbol_editor;
	
	static QCursor* cursor;
};



/** PointSymbolEditorActivity draws a small cross in the origin of the map coordinate system.
 *  FIXME: This cross may cover the symbol at small scales.
 */
class PointSymbolEditorActivity : public MapEditorActivity
{
public:
	PointSymbolEditorActivity(Map* map, PointSymbolEditorWidget* symbol_editor);
	
	virtual void init();
	void update();
	virtual void draw(QPainter* painter, MapWidget* map_widget);
	
private:
	Map* const map;
	PointSymbolEditorWidget* const symbol_editor;
	
	static const int cross_radius; // NOTE: This could be a configuration option.
};

#endif
