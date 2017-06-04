/*
 *    Copyright 2012-2014 Thomas Schöps
 *    Copyright 2013-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_DRAW_PATH_H
#define OPENORIENTEERING_DRAW_PATH_H

#include <QPointer>
#include <QScopedPointer>

#include "draw_line_and_area_tool.h"

class ConstrainAngleToolHelper;
class SnappingToolHelper;
class SnappingToolHelperSnapInfo;
class FollowPathToolHelper;
class KeyButtonBar;

/** 
 * Tool to draw arbitrarily shaped PathObjects.
 */
class DrawPathTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawPathTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool, bool allow_closing_paths);
	virtual ~DrawPathTool();
	
	void init() override;
	const QCursor& getCursor() const override;
	
	bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget) override;
	bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget) override;
	bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget) override;
	bool mouseDoubleClickEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget) override;
	
	bool keyPressEvent(QKeyEvent* event) override;
	bool keyReleaseEvent(QKeyEvent* event) override;
	
	void draw(QPainter* painter, MapWidget* widget) override;
	
protected slots:
	void updateDirtyRect();
	void setDrawingSymbol(const Symbol* symbol) override;
	
	/** This slot listens to changes in the map's object selection. */
	virtual void objectSelectionChanged();
	
protected:
	void updatePreviewPath() override;
	/** Should be called when moving the cursor without the draw button being held */
	void updateHover();
	/** Called by updateHover() if the user is currently drawing */
	void updateDrawHover();
	/** Updates the last three points of the path to form a bezier curve */
	void createPreviewCurve(MapCoord position, float direction);
	/** Closes the preview path */
	void closeDrawing();
	void finishDrawing() override;
	void abortDrawing() override;
	void undoLastPoint();
	/** When not drawing but when the current selection is a single path,
	 *  removes the last point from that path, or deletes the whole path if
	 *  there are too few remaining points.
	 *  Returns true if the path was modified or deleted. */
	bool removeLastPointFromSelectedPath();
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
	
	/** The beginning of the current curve in the preview path. */
	MapCoordF previous_pos_map;  
	/** A control point defining the tangent at the beginning of the current curve. */
	MapCoordF previous_drag_map;
	
	bool dragging;
	
	bool path_has_preview_point;
	bool previous_point_is_curve_point;
	float previous_point_direction;
	bool create_spline_corner; // for drawing bezier splines without parallel handles
	bool create_segment;
	
	/** Information for correct undo on finishing the path by double clicking. */
	bool created_point_at_last_mouse_press;
	
	bool draw_dash_points;
	bool allow_closing_paths;
	
	/** This property is set to true after finishing a path
	 *  and reset to false when the selection changes. */
	bool finished_path_is_selected;
	
	MapWidget* cur_map_widget;
	
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
	bool left_mouse_down;
	bool ctrl_pressed;
	bool picking_angle;   ///< Indicates picking of the initial angle from an object.
	bool picked_angle;    ///< Indicates an active angle picked from another object.
	MapCoordF constrained_pos_map;
	
	QScopedPointer<SnappingToolHelper> snap_helper;
	bool shift_pressed;
	
	bool appending;
	PathObject* append_to_object;
	
	bool following;
	QScopedPointer<FollowPathToolHelper> follow_helper;
	PathObject* follow_object;
	MapCoordVector::size_type follow_start_index;
	
	QPointer<KeyButtonBar> key_button_bar;
};

#endif
