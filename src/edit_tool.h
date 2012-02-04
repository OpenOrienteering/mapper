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

#include "map_editor.h"
#include "object.h"

class CombinedSymbol;
class PointObject;
class PathObject;
class Symbol;

/// Tool to draw point objects
class EditTool : public MapEditorTool
{
Q_OBJECT
public:
	EditTool(MapEditorController* editor, QAction* tool_button);
	virtual ~EditTool();
	
    virtual void init();
    virtual QCursor* getCursor() {return cursor;}
    
    virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
    virtual bool keyPressEvent(QKeyEvent* event);
    virtual bool keyReleaseEvent(QKeyEvent* event);
	
    virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	static QImage* point_handles;
	
protected:
	/// The numbers correspond to the position in point-handles.png
	enum PointHandleType
	{
		StartHandle = 0,
		EndHandle = 1,
		NormalHandle = 2,
		CurveHandle = 3,
		DashHandle = 4
	};
	
	void updateStatusText();
	void updatePreviewObjects();
	void updateDirtyRect();
	void updateHoverPoint(QPointF point, MapWidget* widget);
	void updateDragging(QPoint cursor_pos, MapWidget* widget);
	
	static bool sortObjects(const std::pair<int, Object*>& a, const std::pair<int, Object*>& b);
	bool selectionInfosEqual(const SelectionInfoVector& a, const SelectionInfoVector& b);
	void deleteOldRenderables();
	
	int findHoverPoint(QPointF cursor, Object* object, MapWidget* widget);
	inline float distanceSquared(const QPointF& a, const QPointF& b) {float dx = b.x() - a.x(); float dy = b.y() - a.y(); return dx*dx + dy*dy;}
	void drawPointHandles(QPainter* painter, Object* object, MapWidget* widget);
	void drawPointHandle(QPainter* painter, QPointF point, PointHandleType type, bool active);
	void drawCurveHandleLine(QPainter* painter, QPointF point, QPointF curve_handle, bool active);
	
	// Mouse handling
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
	bool box_selection;
	
	// Information about the selection
	QRectF selection_extent;
	QRectF original_selection_extent;	// before starting an edit operation
	int hover_point;					// path point index if non-negative; if hovering over the extent rect: -1, if hovering over nothing: -2
	int opposite_curve_handle_index;	// -1 if no opposite curve handle
	double opposite_curve_handle_dist;
	int curve_anchor_index;				// if moving a curve handle, this is the index of the point between the handle and its opposite handle, if that exists
	
	// Information about the last click
	SelectionInfoVector last_results;
	SelectionInfoVector last_results_ordered;
	int next_object_to_select;
	
	RenderableVector old_renderables;
	RenderableContainer renderables;
};

#endif
