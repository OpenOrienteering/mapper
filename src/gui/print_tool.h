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


#ifndef _OPENORIENTEERING_PRINT_TOOL_H_
#define _OPENORIENTEERING_PRINT_TOOL_H_

#include <QWidget>

#include "../tool.h"

class PrintWidget;

/**
 * The PrintTool lets the user move see and move the print area on the map.
 */
class PrintTool : public MapEditorTool
{
Q_OBJECT
public:
	PrintTool(MapEditorController* editor, PrintWidget* print_widget);
	
	virtual void init();
	virtual QCursor* getCursor();
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
	void updatePrintArea();
	
private:
	void updateDragging(MapCoordF mouse_pos_map);
	void drawBetweenRects(QPainter* painter, const QRect &outer, const QRect &inner) const;
	
	PrintWidget* print_widget;
	bool dragging;
	MapCoordF click_pos_map;
};

#endif
