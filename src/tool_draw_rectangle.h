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


#ifndef _OPENORIENTEERING_DRAW_RECTANGLE_H_
#define _OPENORIENTEERING_DRAW_RECTANGLE_H_

#include "tool_draw_line_and_area.h"

#include <QScopedPointer>

class ConstrainAngleToolHelper;
class SnappingToolHelper;
class KeyButtonBar;

/**
 * Tool to draw rectangular PathObjects (but also 45 degree angles).
 */
class DrawRectangleTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawRectangleTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool);
    virtual ~DrawRectangleTool();
	
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
	
protected:
	virtual void finishDrawing();
	virtual void abortDrawing();
	
	/**
	 * Deletes the last drawn point.
	 * 
	 * Calls updateRectangle() which will set the than-last point
	 * to constrained_pos_map.
	 */
	void undoLastPoint();
	
	/** Picks a direction from an existing object. */
	void pickDirection(MapCoordF coord, MapWidget* widget);
	
	/** Checks if the current drawing direction is parallel to the angle. */
	bool drawingParallelTo(double angle);
	
	/** 
	 * Updates the preview after cursor position changes.
	 * May call updateRectangle() internally.
	 */
	void updateHover(bool mouse_down);
	
	/**
	 * Recalculates the "close vector"
	 * (direction from current drawing perpendicular to the start point)
	 */
	void updateCloseVector();
	
	/**
	 * Deletes all points from the preview path which were introduced to close
	 * it temporarily (for preview visualization).
	 */
	void deleteClosePoint();
	
	/**
	 * Recalculates the rectangle shape based on the current input.
	 * 
	 * This will set the "last" point to constrained_pos_map.
	 */
	void updateRectangle();
	
	void updateStatusText();
	
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	MapCoordF constrained_pos_map;
	bool dragging;
	bool draw_dash_points;
	bool shift_pressed;
	bool ctrl_pressed;
	bool picked_direction;
	bool snapped_to_line;
	MapCoord snapped_to_line_a;
	MapCoord snapped_to_line_b;
	
	/**
	 * This can be set to true when a mouse button is pressed down to disable all
	 * actions for the next mouse button release.
	 */
	bool no_more_effect_on_click;
	
	/**
	 * List of angles for first, second, etc. edge.
	 * Includes the currently edited angle.
	 * The index of currently edited point in preview_path is angles.size().
	 */
	std::vector< double > angles;
	
	/** Vector in forward drawing direction */
	MapCoordF forward_vector;
	
	/** Direction from current drawing perpendicular to the start point */
	MapCoordF close_vector;
	
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
	QScopedPointer<SnappingToolHelper> snap_helper;
	MapWidget* cur_map_widget;
	
	KeyButtonBar* key_button_bar;
};

#endif
