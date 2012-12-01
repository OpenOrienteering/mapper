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


#ifndef _OPENORIENTEERING_DRAW_PATH_H_
#define _OPENORIENTEERING_DRAW_PATH_H_

#include <QScopedPointer>

#include "tool_draw_line_and_area.h"
#include "tool_helpers.h"

/// Tool to draw path objects
class DrawPathTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawPathTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget, bool allow_closing_paths);
	
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
	void updateHover();
	void updateDrawHover();
	void createPreviewCurve(MapCoord position, float direction);
	void closeDrawing();
	virtual void finishDrawing();
	virtual void abortDrawing();
	void undoLastPoint();
	void updateAngleHelper();
	void updateSnapHelper();
	
	void startAppending(SnappingToolHelper::SnapInfo& snap_info);
	
	void startFollowing(SnappingToolHelper::SnapInfo& snap_info, const MapCoord& snap_coord);
	void updateFollowing();
	void finishFollowing();
	
	float calculateRotation(QPoint mouse_pos, MapCoordF mouse_pos_map);
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
	
	bool space_pressed;
	bool allow_closing_paths;
	
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
	bool left_mouse_down;
	MapCoordF constrained_pos_map;
	
	SnappingToolHelper snap_helper;
	bool shift_pressed;
	MapWidget* cur_map_widget;
	
	bool appending;
	PathObject* append_to_object;
	
	bool following;
	FollowPathToolHelper follow_helper;
	PathObject* follow_object;
	int follow_start_index;
};

#endif
