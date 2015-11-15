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


#ifndef _OPENORIENTEERING_DRAW_CIRCLE_H_
#define _OPENORIENTEERING_DRAW_CIRCLE_H_

#include <QPointer>

#include "tool_draw_line_and_area.h"

class KeyButtonBar;


/** Tool to draw circles and ellipses. */
class DrawCircleTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawCircleTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool);
	virtual ~DrawCircleTool();
	
	virtual void init();
	virtual const QCursor& getCursor() const;
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	virtual bool keyPressEvent(QKeyEvent* event);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
protected:
	virtual void finishDrawing();
	virtual void abortDrawing();
	
	void updateCircle();
	void setDirtyRect();
	void updateStatusText();
	
	QPoint click_pos;
	MapCoordF circle_start_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	MapCoordF opposite_pos_map;		// position on cirlce/ellipse opposite to click_pos_map
	bool dragging;
	bool start_from_center;
	bool first_point_set;
	bool second_point_set;
	QPointer<KeyButtonBar> key_button_bar;
};

#endif
