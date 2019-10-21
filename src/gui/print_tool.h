/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2016  Kai Pastor
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


#ifndef OPENORIENTEERING_PRINT_TOOL_H
#define OPENORIENTEERING_PRINT_TOOL_H

#ifdef QT_PRINTSUPPORT_LIB

#include <QObject>
#include <QPoint>

#include "core/map_coord.h"
#include "tools/tool.h"

class QCursor;
class QMouseEvent;
class QPainter;

namespace OpenOrienteering {

class MapEditorController;
class MapPrinter;
class MapWidget;


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
	 * 
	 *  The parameters must not be null. */
	PrintTool(MapEditorController* editor, MapPrinter* map_printer);
	
	~PrintTool() override;
	
	/** Notifies the tool that it becomes active. */
	void init() override;
	
	/** Always returns the tool's default cursor. */
	const QCursor& getCursor() const override;
	
	/** Starts a dragging interaction. */
	bool mousePressEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	
	/** Updates the state of a running dragging interaction. When not dragging,
	 *  it will update the cursor to indicate a possible interaction. */
	bool mouseMoveEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	
	/** Finishes dragging interactions. */
	bool mouseReleaseEvent(QMouseEvent* event, const MapCoordF& map_coord, MapWidget* widget) override;
	
	/** Draws a visualization of the print area the map widget. */
	void draw(QPainter* painter, MapWidget* widget) override;
	
public slots:
	/** Updates the print area visualization in the map editors. */
	void updatePrintArea();
	
protected:
	/** Modifies the print area while dragging.
	 *  This must not be called when the region is Outside. */
	void updateDragging(const MapCoordF& mouse_pos_map);
	
	/** Updates the current interaction region.
	 *  This must not be called during dragging. */
	void mouseMoved(const MapCoordF& mouse_pos_map, MapWidget* widget);
	
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

#endif // QT_PRINTSUPPORT_LIB


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_PRINT_TOOL_H
