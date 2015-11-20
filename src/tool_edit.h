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


#ifndef _OPENORIENTEERING_EDIT_TOOL_H_
#define _OPENORIENTEERING_EDIT_TOOL_H_

#include <QScopedPointer>

#include "tool.h"
#include "tool_helpers.h"

class MapWidget;
class CombinedSymbol;
class PointObject;
class PathObject;
class Symbol;
class TextObjectEditorHelper;
class Renderable;
class MapRenderables;
class SymbolWidget;
typedef std::vector<Renderable*> RenderableVector;
typedef std::vector< std::pair< int, Object* > > SelectionInfoVector;


/// The standard tool to edit all types of objects
class EditTool : public MapEditorTool
{
Q_OBJECT
public:
	EditTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget);
	virtual ~EditTool();
	
    virtual void init();
    virtual QCursor* getCursor() {return cursor;}
    
    virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
    virtual bool keyPressEvent(QKeyEvent* event);
    virtual bool keyReleaseEvent(QKeyEvent* event);
    virtual void focusOutEvent(QFocusEvent* event);
	
    virtual void draw(QPainter* painter, MapWidget* widget);
	
	inline bool hoveringOverFrame() const {return hover_point == -1;}
	
	static QCursor* cursor;
	
	static const Qt::KeyboardModifiers selection_modifier;
	static const Qt::KeyboardModifiers control_point_modifier;
	static const Qt::Key selection_key;
	static const Qt::Key control_point_key;
	
public slots:
	void objectSelectionChanged();
	void selectedSymbolsChanged();
	void textSelectionChanged(bool text_change);
	
protected slots:
	void updatePreviewObjects(bool force = false);
	void updateDirtyRect();
	
protected:
	void updateStatusText();
	void updateHoverPoint(QPointF point, MapWidget* widget);
	void updateDragging(const MapCoordF& cursor_pos_map);
	bool hoveringOverSingleText(MapCoordF cursor_pos_map);
	
	void startEditing();
	void finishEditing();
	void updateAngleHelper(const MapCoordF& cursor_pos);
	void deleteSelectedObjects();
	
	static bool sortObjects(const std::pair<int, Object*>& a, const std::pair<int, Object*>& b);
	bool selectionInfosEqual(const SelectionInfoVector& a, const SelectionInfoVector& b);
	
	// Mouse handling
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
	bool box_selection;
	bool no_more_effect_on_click;
	
	bool control_pressed;
	bool shift_pressed;
	bool space_pressed;
	
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
	MapCoordF constrained_pos_map;
	QPointF constrained_pos;
	
	// Information about the selection
	QRectF selection_extent;
	QRectF original_selection_extent;	// before starting an edit operation
	int hover_point;					// path point index if non-negative; if hovering over the extent rect: -1, if hovering over nothing: -2
	int opposite_curve_handle_index;	// -1 if no opposite curve handle
	double opposite_curve_handle_dist;
	MapCoord opposite_curve_handle_original_position;
	int curve_anchor_index;				// if moving a curve handle, this is the index of the point between the handle and its opposite handle, if that exists
	std::vector<Object*> undo_duplicates;
	
	TextObjectEditorHelper* text_editor;
	QString old_text;					// to prevent creating an undo step if text edit mode is entered and left, but no text was changed
	int old_horz_alignment;
	int old_vert_alignment;
	QPointF box_text_handles[4];
	
	// Information about the last click
	SelectionInfoVector last_results;
	SelectionInfoVector last_results_ordered;
	int next_object_to_select;
	
	QScopedPointer<MapRenderables> old_renderables;
	QScopedPointer<MapRenderables> renderables;
	SymbolWidget* symbol_widget;
	MapWidget* cur_map_widget;
	
	bool preview_update_triggered;
};

#endif
