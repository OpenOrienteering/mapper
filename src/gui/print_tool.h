/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
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


#ifdef QT_PRINTSUPPORT_LIB

#ifndef _OPENORIENTEERING_PRINT_TOOL_H_
#define _OPENORIENTEERING_PRINT_TOOL_H_

#include <QWidget>

#include "../tool.h"

class MapPrinter;

/**
 * The PrintTool lets the user see and modify the print area on the map
 * by dragging
 * 
 * It interacts with a MapEditorController and a PrintWidget which are set in
 * the constructor.
 */
class PrintTool : public MapEditorTool
{
Q_OBJECT
public:
	/** Constructs a new PrintTool to configure the given map printer in the 
	 *  context of the editor.
	 *  The parameters must not be NULL. */
	PrintTool(MapEditorController* editor, MapPrinter* map_printer);
	
	/** Notifies the tool that it becomes active. */
	virtual void init();
	
	/** Always returns the tool's default cursor. */
	virtual QCursor* getCursor();
	
	/** Starts a dragging interaction. */
	virtual bool mousePressEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	/** Updates the state of a running dragging interaction. When not dragging,
	 *  it will update the cursor to indicate a possible interaction. */
	virtual bool mouseMoveEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	/** Finishes dragging interactions. */
	virtual bool mouseReleaseEvent(QMouseEvent* event, MapCoordF map_coord, MapWidget* widget);
	
	/** Draws a visualization of the print area the map widget. */
	virtual void draw(QPainter* painter, MapWidget* widget);
	
public slots:
	/** Updates the print area visualization in the map editors. */
	void updatePrintArea();
	
protected:
	/** Modifies the print area while dragging.
	 *  This must not be called when the region is Outside. */
	void updateDragging(MapCoordF mouse_pos_map);
	
	/** Updates the current interaction region.
	 *  This must not be called during dragging. */
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
	
	/** The map printer this tool is operation on. */
	MapPrinter* const map_printer;
	
	/** The region of the print area where the current interaction takes place. */
	InteractionRegion region;
	
	/** Indicates whether an interaction is taking place at the moment. */
	bool dragging;
	
	/** The screen position where the initial click was made. */
	QPoint click_pos;
	
	/** The map position where the initial click was made. */
	MapCoordF click_pos_map;
};

#endif

#endif
