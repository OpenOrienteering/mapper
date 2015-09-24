/*
 *    Copyright 2014 Thomas Schöps
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


#ifndef _OPENORIENTEERING_DRAW_FREEHAND_H_
#define _OPENORIENTEERING_DRAW_FREEHAND_H_

#include "tool_draw_line_and_area.h"


/** Tool for free-hand drawing. */
class DrawFreehandTool : public DrawLineAndAreaTool
{
Q_OBJECT
public:
	DrawFreehandTool(MapEditorController* editor, QAction* tool_action, bool is_helper_tool);
	virtual ~DrawFreehandTool();
	
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
	
	void updatePath();
	void setDirtyRect();
	void updateStatusText();
	
	void checkLineSegment(int a, int b, std::vector<bool>& point_mask);
	
	QPoint click_pos;
	MapCoordF last_pos_map;
	QPoint cur_pos;
	MapCoordF cur_pos_map;
	bool dragging;
};

#endif
