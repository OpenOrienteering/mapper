/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_DRAW_PATH_H_
#define _OPENORIENTEERING_DRAW_PATH_H_

#include <QScopedPointer>

#include "tool_draw_line_and_area.h"

class ConstrainAngleToolHelper;
class SnappingToolHelper;
class SnappingToolHelperSnapInfo;
class FollowPathToolHelper;

/** 
 * Tool to draw arbitrarily shaped PathObjects.
 */
class DrawPathTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawPathTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget, bool allow_closing_paths);
	virtual ~DrawPathTool();
	
	virtual void init();
	virtual QCursor* getCursor() {return cursor;}
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	virtual bool keyPressEvent(QKeyEvent* event);
	virtual bool keyReleaseEvent(QKeyEvent* event);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	
protected slots:
	void updateDirtyRect();
	virtual void selectedSymbolsChanged();
	
protected:
    virtual void updatePreviewPath();
	/** Should be called when moving the cursor without the draw button being held */
	void updateHover();
	/** Called by updateHover() if the user is currently drawing */
	void updateDrawHover();
	/** Updates the last three points of the path to form a bezier curve */
	void createPreviewCurve(MapCoord position, float direction);
	/** Closes the preview path */
	void closeDrawing();
	virtual void finishDrawing();
	virtual void abortDrawing();
	void undoLastPoint();
	void updateAngleHelper();
	bool pickAngle(MapCoordF coord, MapWidget* widget);
	void updateSnapHelper();
	
	/** Starts appending to another, existing object */
	void startAppending(SnappingToolHelperSnapInfo& snap_info);
	
	/** Starts following another, existing path */
	void startFollowing(SnappingToolHelperSnapInfo& snap_info, const MapCoord& snap_coord);
	void updateFollowing();
	void finishFollowing();
	
	/**
	 * Checks if the user dragged the mouse away a certain minimum distance from
	 * the click point and if yes, returns the drag angle, otherwise returns 0.
	 */
	float calculateRotation(QPoint mouse_pos, MapCoordF mouse_pos_map);
	/**
	 * Activates or deactivates dash point drawing depending on if a line symbol
	 * with dash symbols is selcted.
	 */
	void updateDashPointDrawing();
	void updateStatusText();
	
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	MapCoordF previous_pos_map;
	MapCoordF previous_drag_map;
	bool dragging;
	
	bool path_has_preview_point;
	bool previous_point_is_curve_point;
	float previous_point_direction;
	bool create_spline_corner; // for drawing bezier splines without parallel handles
	bool create_segment;
	
	bool draw_dash_points;
	bool allow_closing_paths;
	
	MapWidget* cur_map_widget;
	
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
	bool left_mouse_down;
	bool ctrl_pressed;
	bool picking_angle;
	bool picked_angle;
	MapCoordF constrained_pos_map;
	
	QScopedPointer<SnappingToolHelper> snap_helper;
	bool shift_pressed;
	
	bool appending;
	PathObject* append_to_object;
	
	bool following;
	QScopedPointer<FollowPathToolHelper> follow_helper;
	PathObject* follow_object;
	int follow_start_index;
};

#endif
