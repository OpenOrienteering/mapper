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


#ifndef _OPENORIENTEERING_DRAW_CIRCLE_H_
#define _OPENORIENTEERING_DRAW_CIRCLE_H_

#include "tool_draw_line_and_area.h"

/// Tool to draw circles
class DrawCircleTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawCircleTool(MapEditorController* editor, QAction* tool_button, SymbolWidget* symbol_widget);
	
    virtual void init();
    virtual QCursor* getCursor() {return cursor;}
    
    virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
    virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	virtual bool keyPressEvent(QKeyEvent* event);
	
    virtual void draw(QPainter* painter, MapWidget* widget);
	
	static QCursor* cursor;
	
protected:
	virtual void finishDrawing();
	virtual void abortDrawing();
	
	void updateCircle();
	void setDirtyRect();
	void updateStatusText();
	
	QPoint click_pos;
	MapCoordF click_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	MapCoordF previous_pos_map;
	MapCoordF previous_drag_map;
	bool dragging;
	bool first_point_set;
};

#endif
