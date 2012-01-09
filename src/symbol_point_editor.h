/*
 *    Copyright 2011 Thomas Schöps
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

QT_BEGIN_NAMESPACE
class QComboBox;
class QListWidget;
class QPushButton;
class QStackedLayout;
class QLineEdit;
class QTableWidget;
class QCheckBox;
QT_END_NAMESPACE

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
	PointSymbolEditorWidget(Map* map, std::vector< PointSymbol* > symbols, QWidget* parent = 0);
	
	bool changeCurrentCoordinate(MapCoordF new_coord);	// returns if successful
	bool addCoordinate(MapCoordF new_coord);				// returns if successful
	
private slots:
	void updateElementList();
	
	void currentSymbolChanged(int index);
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
	
private:
	struct SymbolInfo
	{
		double origin_x;
		double origin_y;
		PointObject* midpoint_object;
	};
	
	void updateCoordsTable();
	void addCoordsRow(int row);
	void updateCoordsRow(int row);
	void updateDeleteCoordButton();
	
	void insertElement(Object* object, Symbol* symbol);
	QString getLabelForSymbol(Symbol* symbol);
	
	Symbol* getCurrentElementSymbol();
	Object* getCurrentElementObject();
	Object* getMidpointObject();
	
	std::vector< SymbolInfo > symbol_info;
	QComboBox* current_symbol_combo;
	PointSymbol* current_symbol;
	
	QListWidget* element_list;
	QPushButton* delete_element_button;
	
	QStackedLayout* element_properties_layout;
	
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
	
	QTableWidget* coords_table;
	QPushButton* add_coord_button;
	QPushButton* delete_coord_button;
	
	bool react_to_changes;
	Map* map;
};

class PointSymbolEditorTool : public MapEditorTool
{
Q_OBJECT
public:
	PointSymbolEditorTool(MapEditorController* editor, PointSymbolEditorWidget* widget);
	
    virtual void init();
    virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual QCursor* getCursor() {return cursor;};
	
	static QCursor* cursor;
	
private:
	PointSymbolEditorWidget* widget;
};

class PointSymbolEditorActivity : public MapEditorActivity
{
public:
	PointSymbolEditorActivity(Map* map, PointSymbolEditorWidget* widget);
	
    virtual void init();
    virtual void draw(QPainter* painter, MapWidget* widget);
	
private:
	Map* map;
	PointSymbolEditorWidget* widget;
	
	static const int cross_radius;
};

#endif
