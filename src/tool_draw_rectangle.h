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


#ifndef _OPENORIENTEERING_DRAW_RECTANGLE_H_
#define _OPENORIENTEERING_DRAW_RECTANGLE_H_

#include "tool_draw_line_and_area.h"

#include <QScopedPointer>

#include "tool_helpers.h"

/// Tool to draw rectangles
class DrawRectangleTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawRectangleTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget);
	
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
	void undoLastPoint();
	
	void updateRectangle();
	void updatePreview();
	void updateStatusText();
	
	QPoint mouse_press_pos;
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	MapCoordF constrained_pos_map;
	bool dragging;
	bool draw_dash_points;
	
	bool second_point_set;
	bool third_point_set;
	MapCoordF forward_vector;
	MapCoordF close_vector;
	bool new_corner_needed;
	bool delete_start_point;
	
	QScopedPointer<ConstrainAngleToolHelper> angle_helper;
};

#endif
