/*
 *    Copyright 2013 Thomas Schöps
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


#ifndef OPENORIENTEERING_TOUCH_CURSOR_H
#define OPENORIENTEERING_TOUCH_CURSOR_H

#include <QPoint>
#include <QPointF>

#include "core/map_coord.h"
class QMouseEvent;
class QPainter;

namespace OpenOrienteering {

class MapWidget;


/**
 * Handles drawing and controlling a helper cursor inside the map widget for the mobile UI.
 */
class TouchCursor
{
public:
	/** List of IDs for controls attached to the cursor. */
	enum ControlID
	{
		LeftButton = 0,
		
		NoButton
	};
	
	/** Constructs a touch cursor for a map widget. */
	TouchCursor(MapWidget* map_widget);
	
	/**
	 * Notifies the cursor of the event, possibly modifying it.
	 * Attention: the event type may be changed to a mouse move event.
	 */
	void mousePressEvent(QMouseEvent* event);
	
	/**
	 * Notifies the cursor of the event, possibly modifying it.
	 * Returns true if the map widget should further process event.
	 */
	bool mouseMoveEvent(QMouseEvent* event);
	
	/**
	 * Notifies the cursor of the event, possibly modifying it.
	 * Returns true if the map widget should further process event.
	 */
	bool mouseReleaseEvent(QMouseEvent* event);
	
	/**
	 * Notifies the cursor of the event, possibly modifying it.
	 * Returns true if the map widget should further process event.
	 */
	bool mouseDoubleClickEvent(QMouseEvent* event);
	
	/** Paints the cursor. */
	void paint(QPainter* painter);

	/** Issues a (delayed) redraw of the cursor. */
	void updateMapWidget(bool delayed);
	
private:
	/**
	 * Checks if a touch at pos is inside a control. If yes, returns true and
	 * sets out_id to the ID of the touched control.
	 */
	bool touchedControl(const QPoint& pos, ControlID* out_id);
	
	/** Returns the touch point offset from the cursor in pixels. */
	float touchPosOffsetPx() const;
	
	/** Returns the radius of the control ring in pixels */
	float controlRingRadiusPx() const;
	float controlRingStrokeRadiusPx() const;
	
	float standardCursorRadiusPx() const;
	
	bool visible;
	bool left_button_pressed;
	bool first_move_event_received;
	ControlID last_pressed_button;
	MapCoordF cursor_coord;
	QPointF last_cursor_pos;
	QPoint last_touch_pos;
	MapWidget* map_widget;
};


}  // namespace OpenOrienteering

#endif
