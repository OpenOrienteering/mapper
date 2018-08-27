/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_SYMBOL_POINT_EDITOR_H
#define OPENORIENTEERING_SYMBOL_POINT_EDITOR_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QWidget>

#include "core/map_coord.h"
#include "gui/map/map_editor_activity.h"
#include "tools/tool.h"

class QCheckBox;
class QComboBox;
class QCursor;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QMouseEvent;
class QPainter;
class QPushButton;
class QStackedWidget;
class QTableWidget;

namespace OpenOrienteering {

class ColorDropDown;
class Map;
class MapEditorController;
class MapWidget;
class Object;
class PointObject;
class PointSymbol;
class PointSymbolEditorActivity;
class Symbol;


/** A Widget for editing point symbol definitions */
class PointSymbolEditorWidget : public QWidget
{
Q_OBJECT
friend class PointSymbolEditorActivity;
public:
	/** Construct a new widget.
	 * @param controller The controller of the preview map
	 * @param symbol The point symbol to be edited
	 * @param offset_y The vertical offset of the point symbol preview/editor from the origin
	 * @param permanent_preview A flag indicating wheter the preview shall be visible even if the editor is not visible
	 */
	PointSymbolEditorWidget(MapEditorController* controller, PointSymbol* symbol, qreal offset_y = 0, bool permanent_preview = false, QWidget* parent = 0);
	
	~PointSymbolEditorWidget() override;
	
	/** Add a coordinate to the current element.
	 *  @return true if successful
	 */
	bool addCoordinate(const MapCoordF& new_coord);
	
	/** Change the current coordinate of the current element.
	 *  @return true if successful
	 */
	bool changeCurrentCoordinate(const MapCoordF& new_coord);
	
	/** Activate the editor in the map preview. */
	void setEditorActive(bool active);
	
	/** Request to hide or show the editor. */
	void setVisible(bool visible) override;
	
signals:
	/** This signal gets emitted whenever the symbol appearance is modified. */
	void symbolEdited();
	
private slots:
	void orientedToNorthClicked(bool checked);
	
	void changeElement(int row);
	
	void addPointClicked();
	void addLineClicked();
	void addAreaClicked();
	void deleteCurrentElement();
	void centerAllElements();
	
	void pointInnerRadiusChanged(double value);
	void pointInnerColorChanged();
	void pointOuterWidthChanged(double value);
	void pointOuterColorChanged();
	
	void lineWidthChanged(double value);
	void lineColorChanged();
	void lineCapChanged(int index);
	void lineJoinChanged(int index);
	void lineClosedClicked(bool checked);
	
	void areaColorChanged();
	
	void updateDeleteCoordButton();
	void coordinateChanged(int row, int column);
	void addCoordClicked();
	void deleteCoordClicked();
	void centerCoordsClicked();
	
private:
	void initElementList();
	void updateCoordsTable();
	void addCoordsRow(int row);
	void updateCoordsRow(int row);
	
	void insertElement(Object* object, Symbol* symbol);
	QString getLabelForSymbol(const Symbol* symbol) const;
	
	Symbol* getCurrentElementSymbol();
	Object* getCurrentElementObject();
	
	PointSymbol* const symbol;
	PointObject* midpoint_object;
	const MapCoordF object_origin_coord;
	
	QCheckBox* oriented_to_north;
	
	QListWidget* element_list;
	QPushButton* delete_element_button;
	QPushButton* center_all_elements_button;
	
	QStackedWidget* element_properties_widget;
	
	QWidget* point_properties;
	QDoubleSpinBox* point_inner_radius_edit;
	ColorDropDown* point_inner_color_edit;
	QDoubleSpinBox* point_outer_width_edit;
	ColorDropDown* point_outer_color_edit;
	
	QWidget* line_properties;
	QDoubleSpinBox* line_width_edit;
	ColorDropDown* line_color_edit;
	QComboBox* line_cap_edit;
	QComboBox* line_join_edit;
	QCheckBox* line_closed_check;
	
	QWidget* area_properties;
	ColorDropDown* area_color_edit;
	
	QLabel* coords_label;
	QTableWidget* coords_table;
	QPushButton* add_coord_button;
	QPushButton* delete_coord_button;
	QPushButton* center_coords_button;
	
	const qreal offset_y;
	PointSymbolEditorActivity* activity;
	Map* map;
	MapEditorController* controller;
	const bool permanent_preview;
};



/**
 * PointSymbolEditorActivity allows to add or modify coordinates of point symbol elements
 * by clicking in the map.
 */
class PointSymbolEditorTool : public MapEditorTool
{
Q_OBJECT
	
public:
	PointSymbolEditorTool(MapEditorController* editor, PointSymbolEditorWidget* symbol_editor);
	~PointSymbolEditorTool() override;
	
	void init() override;
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* map_widget) override;
	const QCursor& getCursor() const override;
	
private:
	PointSymbolEditorWidget* const symbol_editor;
};



/**
 * PointSymbolEditorActivity draws a small cross in the origin of the map coordinate system.
 * 
 * \todo Fix that thes cross may cover the symbol at small scales.
 */
class PointSymbolEditorActivity : public MapEditorActivity
{
	Q_OBJECT
	
public:
	PointSymbolEditorActivity(Map* map, PointSymbolEditorWidget* symbol_editor);
	~PointSymbolEditorActivity() override;
	
	void init() override;
	void update();
	void draw(QPainter* painter, MapWidget* map_widget) override;
	
private:
	Map* const map;
	PointSymbolEditorWidget* const symbol_editor;
	
	static const int cross_radius; // NOTE: This could be a configuration option.
};


}  // namespace OpenOrienteering

#endif
