/*
 *    Copyright 2012 Thomas Schöps, Kai Pastor
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

class MapPrinter;

/**
 * The PrintTool lets the user see and modify the print area on the map.
 * 
 * It interacts with a MapEditorController and a PrintWidget which are set in
 * the constructor.
 */
class PrintTool : public MapEditorTool
{
Q_OBJECT
public:
	PrintTool(MapEditorController* editor, MapPrinter* map_printer);
	
	virtual void init();
	
	virtual QCursor* getCursor();
	
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	virtual void draw(QPainter* painter, MapWidget* widget);
	
public slots:
	/** Updates the print area visualization in the map editors. */
	void updatePrintArea();
	
protected:
	/** Modifies the print area while dragging. */
	void updateDragging(MapCoordF mouse_pos_map);
	
	/** Updates the current interaction region. */
	void mouseMoved(MapCoordF mouse_pos_map, MapWidget* widget);
	
	/** Regions of interaction with the print area. */
	enum InteractionRegion {
		Inside            = 0x00,
		Outside           = 0x01,
		LeftBorder        = 0x02,
		TopLeftCorner     = 0x06,
		TopBorder         = 0x04,
		TopRightCorner    = 0x0c,
		RightBorder       = 0x08,
		BottomRightCorner = 0x18,
		BottomBorder      = 0x10,
		BottomLeftCorner  = 0x12,
		Unknown           = 0xFF
	};
	
	MapPrinter* map_printer;
	InteractionRegion region;
	bool dragging;
	MapCoordF click_pos_map;
};

#endif
